/* 
 * Calypso block device driver
 * Block driver implementation responsible for reading 
 * and writing data to the shadow partition
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/genhd.h> /* allow partitions and gendisk */
#include <linux/blkdev.h>
#include <linux/errno.h>
#include <linux/blk-mq.h>
/* ioctl */
#include <linux/hdreg.h>
#include <linux/blkpg.h>
/* remap io requests */
#include <trace/events/block.h>
#include <linux/kthread.h>
 
#include "../../lib/global.h"
#include "../../lib/debug.h"
#include "../../lib/disk_io.h"
#include "../../lib/file_io.h"
#include "../../lib/ram_io.h"
#include "../../lib/virtual_device.h"
#include "../../lib/physical_device.h"
#include "../../lib/fs_utils.h"

 
static int calypso_dev_open(struct block_device *bdev, fmode_t mode);

static void calypso_dev_close(struct gendisk *disk, fmode_t mode);

int calypso_dev_ioctl(struct block_device *bdev, fmode_t mode, unsigned cmd, unsigned long arg);

static blk_qc_t calypso_make_request(struct request_queue *q, struct bio *bio);
//static blk_qc_t calypso_make_request(struct bio *bio);

static blk_status_t calypso_dev_request(struct blk_mq_hw_ctx *hctx, const struct blk_mq_queue_data* bd);

enum {
	RAM_MODE = 0,	/* Simulates RAM block device, changes are not persistent */
	FILE_MODE = 1,	/* Writes changes to a file */
	DISK_MODE = 2,	/* Writes changes to disk */
};
static int io_mode = RAM_MODE;
module_param(io_mode, int, 0);

static int request_counter = 0;

static struct calypso_blk_device *calypso_dev = NULL;

static struct ram_device *ram_dev = NULL;

static struct file *calypso_file_read = NULL;
static struct file *calypso_file_write = NULL;

/* pointer to physical device structure */
static struct block_device *physical_dev;

/*
 *
 */
static struct block_device_operations calypso_fops =
{
    .owner = THIS_MODULE,
    .open = calypso_dev_open,
    .release = calypso_dev_close,
    .ioctl = calypso_dev_ioctl,
#if LINUX_VERSION_CODE > KERNEL_VERSION(5,8,0)
	.submit_bio = calypso_make_request,
#endif
};


/* 
 * These are the file operations to perform read and writes
 * to the device
 */
static struct blk_mq_ops calypso_fops_req =
{
    .queue_rq = calypso_dev_request,
};

static blk_status_t snoop_physical_dev_request(struct blk_mq_hw_ctx *hctx, const struct blk_mq_queue_data* bd)
{
	debug_args(KERN_INFO, __func__, "Snooping on requests to %s\n", PHYSICAL_DISK_NAME);

	return 0;
}


static DECLARE_WAIT_QUEUE_HEAD(req_event);

/* 
 * Actual remapping of I/O requests
 */
static void remap_io_request_to_physical_dev(struct bio *bio)
{
//    printk("stackdb: Mapping sector: %llu -> %llu, dev: %s -> %s\n",
//            bio->bi_sector,
//            lba != EMPTY_REAL_LBA ? lba : bio->bi_sector,
//            bio->bi_bdev->bd_disk->disk_name,
//            bdev_raw->bd_disk->disk_name);
//
//    if (lba != EMPTY_REAL_LBA)
//        bio->bi_sector = lba;
    
	debug_args(KERN_INFO, __func__, "Remapping requests from calypso device to %s", PHYSICAL_DISK_NAME);

	bio_set_dev(bio, calypso_dev->physical_dev);

	/* map request from calypso logical device to the physical device */
    trace_block_bio_remap(bdev_get_queue(calypso_dev->physical_dev), bio,
            bio_dev(bio), bio->bi_iter.bi_sector);

    generic_make_request(bio);
	//submit_bio_noacct(bio);
}

/*
 * Handle an I/O request
 */
// #if LINUX_VERSION_CODE <= KERNEL_VERSION(5,8,0)
// static blk_qc_t calypso_make_request(struct bio *bio)
// #else
static blk_qc_t calypso_make_request(struct request_queue *q, struct bio *bio)
// #endif
{
	debug(KERN_INFO, __func__, "Processing requests...");
	//request_counter++;
	//if (request_counter % 10 == 0)
		//debug_args(KERN_INFO, __func__, "Processed %d requests", request_counter);

    // debug_args(KERN_INFO, __func__, "Make request %-5s block %-12llu #pages %-4hu total-size \"
    //         \"%-10u\n", bio_data_dir(bio) == WRITE ? "write" : "read",
    //         bio->bi_sector, bio->bi_vcnt, bio->bi_size);

//    printk("<%p> Make request %s %s %s\n", bio,
//           bio->bi_rw & REQ_SYNC ? "SYNC" : "",
//           bio->bi_rw & REQ_FLUSH ? "FLUSH" : "",
//           bio->bi_rw & REQ_NOIDLE ? "NOIDLE" : "");
//
    //spin_lock_irq(&stackbd.lock);
    // if (!calypso_dev->physical_dev)
    // {
    //     debug(KERN_INFO, __func__, "Request before bdev_raw is ready, aborting\n");
    //     goto abort;
    // }
    // if (!stackbd.is_active)
    // {
    //     printk("stackbd: Device not active yet, aborting\n");
    //     goto abort;
    // }
    //bio_list_add(&(calypso_dev->bio_list), bio);
    //wake_up(&req_event);
    //spin_unlock_irq(&stackbd.lock);

	remap_io_request_to_physical_dev(bio);
	//wake_up(&req_event);

    return 0;

// abort:
//     //spin_unlock_irq(&stackbd.lock);
//     printk("<%p> Abort request\n\n", bio);
//     bio_io_error(bio);
// 	return -1;
}

// TODO: make these call the original functions
static int calypso_dev_open(struct block_device *bdev, fmode_t mode)
{
    debug(KERN_INFO, __func__, "Device is opened\n");

    // struct rb_device *dev = bdev->bd_disk->private_data;
    
    // spin_lock(&dev->lock);
    // dev->users++;
    // spin_unlock(&dev->lock);

    return 0;
}

static void calypso_dev_close(struct gendisk *disk, fmode_t mode)
{
    // struct sbull_dev *dev = disk->private_data;

    // spin_lock(&dev->lock);
	// dev->users--;
    // spin_unlock(&dev->lock);

    debug(KERN_INFO, __func__, "Device is closed\n");
}

int calypso_dev_ioctl(struct block_device *bdev, fmode_t mode, 
						unsigned cmd, unsigned long arg)
{
    debug_args(KERN_INFO, __func__, "ioctl call, cmd: %d\n", cmd);

	switch(cmd) {
		/* retrieve disk geometry */
	    case HDIO_GETGEO:
        /*
		 * Get geometry: since we are a virtual device, we have to make
		 * up something plausible. So we claim 16 sectors, four heads,
		 * and calculate the corresponding number of cylinders.  We set the
		 * start of data at sector four.
		 */
			debug(KERN_INFO, __func__, "HDIO_GETGEO\n");
			break;

		default:
			debug(KERN_INFO, __func__, "default\n");

	}

    return -ENOTTY;
}

/*
 * Processes a single segment from a bio
 */
static int calypso_dev_process_segment(struct request *req,
										struct bio_vec bv, 
										u8 *req_buf, 
										sector_t sector_offset, 
										int dir)
{
	//int ret = 0;

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

	u8 *req_buf2;

	/* bio_vector is a vector of <page, offset, len> */
	// TODO check if they print the same address with %p
	req_buf2 = page_address(bv.bv_page) + bv.bv_offset;
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
	debug_args(KERN_INFO, __func__, "Start Sector: %llu, Sector Offset: %llu; Buffer: %p; Buffer2: %p; Length: %u sectors\n",
		(unsigned long long)(start_sector), (unsigned long long)(sector_offset), req_buf, req_buf2, sectors);

	/* offset represents the place in the buffer we want to start reading from
     or writing to */
    offset_bytes = (start_sector + sector_offset) * DISK_SECTOR_SIZE;
    /* nbytes represents the amount of bytes we are supposed to read from or
    to write to our block device */
    len_bytes = sectors * DISK_SECTOR_SIZE;

	debug_args(KERN_INFO, __func__, "EACH SEGMENT -> SECTOR TO START: %llu, PROCESSING %llu SECTORS;\n",
		(unsigned long long)(start_sector + sector_offset), (unsigned long long)(sectors));

	if (dir == WRITE) /* Write to the device */
	{
		debug(KERN_INFO, __func__, "WRITE");
		debug_args(KERN_INFO, __func__, "writing \"%s\"", req_buf);
		//debug_args(KERN_INFO, __func__, "writing \"%u\"", req_buf[0]);
		switch (io_mode) 
		{
	    	case RAM_MODE:
				calypso_write_segment_to_ram(ram_dev->data, offset_bytes, len_bytes, req_buf);
				break;
			case FILE_MODE:
				//write_segment_to_file(calypso_file_write, offset_bytes, len_bytes, req_buf);
				break;
			case DISK_MODE:
				// write_segment_to_disk(next_sector_to_write, sectors, buffer);
				break;
		}
	}
	else /* Read from the device */
	{
		debug(KERN_INFO, __func__, "READ");
		switch (io_mode) 
		{
	    	case RAM_MODE:
				calypso_read_segment_from_ram(ram_dev->data, offset_bytes, len_bytes, req_buf);
				break;
			case FILE_MODE:
				//calypso_read_segment_from_file(calypso_file_read, offset_bytes, len_bytes, req_buf);
				break;
			case DISK_MODE:
				// calypso_read_segment_from_disk(start_sector + sector_offset, sectors, buffer);
				break;
		}
		debug_args(KERN_INFO, __func__, "read \"%s\"", req_buf);
		//debug_args(KERN_INFO, __func__, "read \"%u\"", req_buf[0]);
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
static int calypso_dev_iterate_request_bios(struct request *req)
{
	//struct calypso_blk_device *dev = (struct calypso_blk_device *)(req->rq_disk->private_data);
	//sector_t *next_sector_to_write = &(dev->next_sector_to_write);

	unsigned int sector_cnt = blk_rq_sectors(req);

	int dir = rq_data_dir(req);

	struct bio_vec bv;
	struct req_iterator iter;

	sector_t sector_offset;
	
	u8 *buffer;

 	int ret = 0;

	debug(KERN_INFO, __func__, "Before iterating through request\n");

	debug_args(KERN_INFO, __func__, "This request is going to process %d sectors\n", sector_cnt);

	/*
	 * Open Calypso file
	 * This can be done here because each request 
	 * is either all for writes or reads 
	 */
	if (io_mode == FILE_MODE) 
	{
		if (dir == WRITE) 
		{
			calypso_file_write = calypso_open_file(CALYPSO_FILE_PATH, O_RDWR | O_CREAT, 0);
			if (!calypso_file_write) {
				debug(KERN_ERR, __func__, "Could not open Calypso file for write\n");
				return -ENOENT;
			}
		}
		else 
		{
			//calypso_file_read = open_file(CALYPSO_FILE_PATH, O_RDONLY | O_CREAT, 0);
			calypso_file_read = calypso_open_file(CALYPSO_FILE_PATH, O_RDWR | O_CREAT, 0);
			if (!calypso_file_read) {
				debug(KERN_ERR, __func__, "Could not open Calypso file for read\n");
				return -ENOENT;
			}
		}
		debug(KERN_INFO, __func__, "Opened Calypso file\n");
	}

	//printk(KERN_DEBUG "rb: Dir:%d; Sec:%lld; Cnt:%d\n", dir, start_sector, sector_cnt);

	sector_offset = 0;
	/* loops through bios and through the segments of each bio */
	rq_for_each_segment(bv, req, iter)
	{
		debug_args(KERN_INFO, __func__, "iter.iter.bi_sector: %lld\n", iter.iter.bi_sector);
		sector_offset = calypso_dev_process_segment(req, bv, buffer, sector_offset, dir);
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

	/* Close Calypso file */
	if (io_mode == FILE_MODE) 
	{
		if (dir == WRITE) 
		{
			calypso_close_file(calypso_file_write);
			debug(KERN_INFO, __func__, "Closed Calypso file for write\n");
		}
		else 
		{
			calypso_close_file(calypso_file_read);
			debug(KERN_INFO, __func__, "Closed Calypso file for read\n");
		}
	}

	return ret;
}
     
/*
 * Processes a single request each time
 * 
 * Represents a block I/O request for us to execute
 * This function should be non blocking, since it's
 * called from a non-process context.
 * This function also takes the queue's spinlock,
 * so it should not be messed with
 */
static blk_status_t calypso_dev_request(struct blk_mq_hw_ctx *hctx, const struct blk_mq_queue_data* bd)
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
    ret = calypso_dev_iterate_request_bios(req);

done:
	blk_mq_end_request(req, ret);
	debug(KERN_DEBUG, __func__, "Ended request\n");
	return ret;
}
     
/* 
 * This is the registration and initialization section of the block device
 * driver
 */
static int __init calypso_init(void)
{
    int ret = 0;
	
	debug_args(KERN_INFO, __func__, "Initializing Calypso in mode %d\n", io_mode);

	ret = calypso_dev_init(&calypso_dev);
	if (ret != 0)
    {
        debug(KERN_ERR, __func__, "Unable to initialize Calypso\n");
		return ret;
    } else {
		debug(KERN_INFO, __func__, "Calypso driver is initialized\n");
	}

	calypso_dev->capacity = CALYPSO_DEVICE_SIZE;

	switch (io_mode) 
	{
		case RAM_MODE:
			ret = calypso_init_ram(&ram_dev, CALYPSO_DEVICE_SIZE);
			if (ret != 0)
			{
				debug(KERN_ERR, __func__, "Unable to initialize RAM device\n");
				return ret;
			} else {
				debug(KERN_INFO, __func__, "RAM device is initialized\n");
			}
			break;
		case FILE_MODE:
			
			break;
		case DISK_MODE:
			
			break;
	}	

    //ret = calypso_dev_setup(calypso_dev, &calypso_fops, &calypso_fops_req);
	ret = calypso_dev_no_queue_setup(calypso_dev, &calypso_fops, CALYPSO_DEV_NAME, &(calypso_dev->major), calypso_make_request);
    if (ret != 0)
    {
        debug(KERN_ERR, __func__, "Unable to set up Calypso\n");
		return ret;
    } else {
		debug(KERN_INFO, __func__, "Calypso driver is set up\n");
	}

	//physical_dev = calypso_open_physical_disk(PHYSICAL_DISK_NAME, calypso_dev);
	calypso_dev->physical_dev = calypso_open_physical_disk(PHYSICAL_DISK_NAME, calypso_dev);
	if (!calypso_dev->physical_dev) {
		debug_args(KERN_ERR, __func__, "Unable to open physical disk %s\n", PHYSICAL_DISK_NAME);
		return -ENOENT;
	}
	else
		debug_args(KERN_INFO, __func__, "Opened physical disk %s\n", PHYSICAL_DISK_NAME);

	calypso_link_physical_virtual_disk(calypso_dev);

	debug_args(KERN_DEBUG, __func__, "physical dev make_request_fn: %p", bdev_get_queue(calypso_dev->physical_dev)->make_request_fn);

	// calypso_partition_fs_info(calypso_dev->physical_dev);
	
	// calypso_iterate_partition_blocks_groups(calypso_dev->physical_dev);

    return ret;

//error_after_bdev:

}
/*
 * This is the unregistration and uninitialization section of the ram block
 * device driver
 */
static void __exit calypso_cleanup(void)
{
	kthread_stop(calypso_dev->snooping_thread);

	//calypso_restore_io_requests_to_physical_dev();

    calypso_close_physical_disk(physical_dev);

	calypso_dev_physical_cleanup(calypso_dev, CALYPSO_DEV_NAME, &(calypso_dev->major));

    //calypso_dev_cleanup(calypso_dev);

	switch (io_mode) 
	{
		case RAM_MODE:
			calypso_free_ram(ram_dev);
			break;
		case FILE_MODE:
			
			break;
		case DISK_MODE:
			
			break;
	}
	

    debug(KERN_DEBUG, __func__, "Calypso driver cleanup");
}
 
module_init(calypso_init);
module_exit(calypso_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Daniela Lopes");
MODULE_DESCRIPTION("Calypso Driver");