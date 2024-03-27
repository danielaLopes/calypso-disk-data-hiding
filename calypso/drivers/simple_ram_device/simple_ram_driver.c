/* 
 * Simple RAM block device driver
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/genhd.h> /* allow partitions and gendisk */
#include <linux/blkdev.h>
#include <linux/errno.h>
#include <linux/blk-mq.h>
 
#include "../global.h"
#include "../debug.h"
#include "../ram_io.h"


#define DEV_NAME "ram_simple"


static blk_qc_t ram_dev_submit_bio(struct bio *bio);
 
static int ram_dev_open(struct block_device *bdev, fmode_t mode);

static void ram_dev_close(struct gendisk *disk, fmode_t mode);

int ram_dev_ioctl(struct block_device *bdev, fmode_t mode, unsigned cmd, unsigned long arg);

static blk_status_t ram_dev_request(struct blk_mq_hw_ctx *hctx, const struct blk_mq_queue_data* bd);

static int major = 0;

/* 
 * The internal structure representation of our device
 */
struct ram_blk_device
{
    /* Size of the device (in sectors) */
    sector_t capacity;

    u8 *data;

    /* For exclusive access to our request queue */
    spinlock_t lock;

    struct blk_mq_tag_set tag_set;	
    /* Our request queue */
    struct request_queue *queue;
    /* This is kernel's representation of an individual disk device */
    struct gendisk *disk;
};

static struct ram_blk_device *ram_dev = NULL;

static struct ram_device *ram_data = NULL;


static struct block_device_operations ram_dev_fops =
{
    .owner = THIS_MODULE,
    .open = ram_dev_open,
    .release = ram_dev_close,
    .ioctl = ram_dev_ioctl,
    .submit_bio = ram_dev_submit_bio,
};

/* 
 * These are the file operations to perform read and writes
 * to the device
 */
static struct blk_mq_ops ram_dev_fops_req =
{
    .queue_rq = ram_dev_request,
};

void memory_cleanup(struct ram_blk_device *ram_dev)
{
    if (ram_dev) {
        if (ram_dev->data) {
            kfree(ram_dev->data);
            ram_dev->data = NULL;
            ram_dev->capacity = 0;
        }
        kfree(ram_dev);
        ram_dev = NULL;
    }

    debug(KERN_INFO, __func__, "RAM memory cleanup done\n");
}

static int ram_dev_process_segment(struct request *req,
										struct bio_vec bv, 
										u8 *req_buf, 
										sector_t sector_offset, 
										int dir)
{
	sector_t start_sector = blk_rq_pos(req);

	/*
	 * blk_rq_pos()         : the current sector
     * blk_rq_bytes()       : bytes left in the entire request
 	 * blk_rq_cur_bytes()   : bytes left in the current segment
	 * blk_rq_err_bytes()   : bytes left till the next error boundary
	 * blk_rq_sectors()     : sectors left in the entire request
	 * blk_rq_cur_sectors() : sectors left in the current segment
	 */
	unsigned int sector_cnt = blk_rq_sectors(req);
	unsigned int sector_cnt_bytes = blk_rq_bytes(req);
	unsigned int sector_cur_cnt = blk_rq_cur_sectors(req);
	unsigned int sector_cur_cnt_bytes = blk_rq_cur_bytes(req);

	unsigned int sectors;

	unsigned long offset_bytes;
	unsigned long len_bytes;

	/* bio_vector is a vector of <page, offset, len> */
	req_buf = page_address(bv.bv_page + bv.bv_offset);
	if (bv.bv_len % DISK_SECTOR_SIZE != 0)
	{
		debug_args(KERN_ERR, __func__, "Should never happen: "
			"bio size (%d) is not a multiple of DISK_SECTOR_SIZE (%d).\n"
			"This may lead to data truncation.\n",
			bv.bv_len, DISK_SECTOR_SIZE);
		return -EIO;
	}
	sectors = bv.bv_len / DISK_SECTOR_SIZE;
	debug_args(KERN_INFO, __func__, "Length:\n sectors: %u\n sector_cnt:%u\n sector_cnt_bytes: %u\n sector_cur_cnt: %u\n sector_cur_cnt_bytes: %u\n",
	sectors, sector_cnt, sector_cnt_bytes, sector_cur_cnt, sector_cur_cnt_bytes);

	/* offset represents the place in the buffer we want to start reading from
     or writing to */
    offset_bytes = (start_sector + sector_offset) * DISK_SECTOR_SIZE;
    /* nbytes represents the amount of bytes we are supposed to read from or
    to write to our block device */
    len_bytes = sectors * DISK_SECTOR_SIZE;

	if (dir == WRITE) /* Write to the device */
	{
		debug(KERN_INFO, __func__, "WRITE");
        memcpy(ram_dev->data + offset_bytes, req_buf, len_bytes);
	}
	else /* Read from the device */
	{
		debug(KERN_INFO, __func__, "READ");
        memcpy(req_buf, ram_dev->data + offset_bytes, len_bytes);
	}

	sector_offset += sectors;
	debug_args(KERN_INFO, __func__, "Incremented sectors by %d", sectors);

	return sector_offset;
}

/* 
 * Extracts every field from a struct request
 * to proceed to the actual data transfer
 * 
 * Parses the struct request, which contains the
 * direction of data transfer (read or write),
 * starting sector for the data transfer,
 * total number of sectors for data transfer,
 * and the buffer to put the results on
 */
static int ram_dev_iterate_request_bios(struct request *req)
{
	unsigned int sector_cnt = blk_rq_sectors(req);

	int dir = rq_data_dir(req);

	struct bio_vec bv;
	struct req_iterator iter;

	sector_t sector_offset;
	
	u8 *buffer;

 	int ret = 0;

	debug_args(KERN_INFO, __func__, "This request is going to process %d sectors\n", sector_cnt);

	sector_offset = 0;
	/* loops through bios and through the segments of each bio */
	rq_for_each_segment(bv, req, iter)
	{
		debug_args(KERN_INFO, __func__, "iter.iter.bi_sector: %lld\n", iter.iter.bi_sector);
		sector_offset = ram_dev_process_segment(req, bv, buffer, sector_offset, dir);
	}
	if (sector_offset < 0)
	{
		debug(KERN_ERR, __func__, "Error processing segment");
		ret = -EIO;
	}
	if (sector_offset != sector_cnt)
	{
		debug(KERN_ERR, __func__, "bio info doesn't match with the request info");
		ret = -EIO;
	}

	return ret;
}

static blk_status_t ram_dev_request(struct blk_mq_hw_ctx *hctx, const struct blk_mq_queue_data* bd)
{
    struct request *req = bd->rq;
    blk_status_t ret;

	debug(KERN_INFO, __func__, "Processing new request\n");

    blk_mq_start_request(req);

    /* skips non-fs requests */
	if (blk_rq_is_passthrough(req)) {
		debug(KERN_NOTICE, __func__, "Skip non-fs request\n");
		ret = BLK_STS_IOERR;  //-EIO
		goto done;
	}
    /* process a single request */
    ret = ram_dev_iterate_request_bios(req);

done:
	blk_mq_end_request(req, ret);
	debug(KERN_DEBUG, __func__, "Ended request\n");
	return ret;
}

// TODO: make these call the original functions
static int ram_dev_open(struct block_device *bdev, fmode_t mode)
{
    debug(KERN_INFO, __func__, "Device is opened\n");

    return 0;
}

static blk_qc_t ram_dev_submit_bio(struct bio *bio)
{
    debug(KERN_INFO, __func__, "Submitting dev\n");

    return blk_mq_submit_bio(bio);
}

static void ram_dev_close(struct gendisk *disk, fmode_t mode)
{
    debug(KERN_INFO, __func__, "Device is closed\n");
}

int ram_dev_ioctl(struct block_device *bdev, fmode_t mode, 
						unsigned cmd, unsigned long arg)
{
    debug_args(KERN_INFO, __func__, "ioctl call, cmd: %d\n", cmd);

    return -ENOTTY;
}
     
/* 
 * This is the registration and initialization section of the block device
 * driver
 */
static int __init ram_dev_init(void)
{
    int ret = 0;

	ram_dev = kzalloc(sizeof(struct ram_blk_device), GFP_KERNEL);
	if (!ram_dev) {
        debug(KERN_WARNING, __func__, "Unable to allocate bytes for RAM device\n");
        return -ENOMEM;
    }
    debug(KERN_INFO, __func__, "Allocated memory for device struct\n");

	ram_dev->data = kmalloc(CALYPSO_DEVICE_SIZE * DISK_SECTOR_SIZE, GFP_KERNEL);
    if (!ram_dev->data) {
		debug(KERN_WARNING, __func__, "Unable to allocate data buffer for RAM device\n");
		kfree(ram_dev);
        ram_dev = NULL;
		return -ENOMEM;
	}  
	debug(KERN_INFO, __func__, "Allocated memory buffer for device\n");

	ram_dev->capacity = CALYPSO_DEVICE_SIZE;

    ret = calypso_init_ram(&ram_data, CALYPSO_DEVICE_SIZE);
    if (ret != 0)
    {
        debug(KERN_ERR, __func__, "Unable to initialize RAM device\n");
        return ret;
    } else {
        debug(KERN_INFO, __func__, "RAM device is initialized\n");
    }

    major = register_blkdev(major, DEV_NAME);
    if (major <= 0)
    {
        debug(KERN_ERR, __func__, "Unable to get Major Number\n");
        return -EBUSY;
    }
    debug_args(KERN_INFO, __func__, "Registered block device with major %d\n", major); 

    spin_lock_init(&(ram_dev->lock));
    ram_dev->queue = blk_mq_init_sq_queue(&(ram_dev->tag_set), &ram_dev_fops_req, 128, BLK_MQ_F_SHOULD_MERGE);
    if (IS_ERR(ram_dev->queue)) {
        debug(KERN_ERR, __func__, "blk_mq_init_sq_queue failure\n");
        memory_cleanup(ram_dev);
        unregister_blkdev(major, DEV_NAME);
        return -ENOMEM;
    }
    /* sets logical block size for the queue */
    blk_queue_logical_block_size(ram_dev->queue, KERNEL_SECTOR_SIZE);
    
	ram_dev->queue->queuedata = ram_dev;

    debug(KERN_INFO, __func__, "Set up request queue\n");

    ram_dev->disk = alloc_disk(1);
    if (!ram_dev->disk)
    {
        debug(KERN_ERR, __func__, "alloc_disk failure\n");
        blk_cleanup_queue(ram_dev->queue);
        memory_cleanup(ram_dev);
        unregister_blkdev(major, DEV_NAME);
        return -ENOMEM;
    }
    debug(KERN_INFO, __func__, "Allocated disk\n");

    /* can only have one partition 
    otherwise we need to create a partition table */
    ram_dev->disk->flags |= GENHD_FL_NO_PART_SCAN;

    /* Setting the major number */
    ram_dev->disk->major = major;
    /* Setting the first mior number */
    ram_dev->disk->first_minor = 0;
    /* Initializing the device operations */
    ram_dev->disk->fops = &ram_dev_fops;
    /* Driver-specific own internal data */
    ram_dev->disk->private_data = ram_dev;
    ram_dev->disk->queue = ram_dev->queue;

    sprintf(ram_dev->disk->disk_name, "ram_simple0");

    set_capacity(ram_dev->disk, ram_dev->capacity);
    debug(KERN_INFO, __func__, "Set disk capacity\n");
 
    add_disk(ram_dev->disk);
    debug(KERN_INFO, __func__, "RAM device initialised\n");

    return ret;
}

/*
 * This is the unregistration and uninitialization section of the ram block
 * device driver
 */
static void __exit ram_dev_cleanup(void)
{
	if (ram_dev) {
        if (ram_dev->disk) {
            del_gendisk(ram_dev->disk);
            put_disk(ram_dev->disk);
            ram_dev->disk = NULL;
            debug(KERN_INFO, __func__, "Deleted disk\n");
        }
        if (ram_dev->queue) {
            blk_cleanup_queue(ram_dev->queue);
            ram_dev->queue = NULL;
            debug(KERN_INFO, __func__, "Deleted request queue\n");
        }
    }

    memory_cleanup(ram_dev);
    
    unregister_blkdev(major, DEV_NAME);

    debug(KERN_INFO, __func__, "RAM device cleanup finished\n");

	calypso_free_ram(ram_data);

    debug(KERN_DEBUG, __func__, "RAM device cleanup");
}
 
module_init(ram_dev_init);
module_exit(ram_dev_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Daniela Lopes");
MODULE_DESCRIPTION("Simple RAM Device Driver");