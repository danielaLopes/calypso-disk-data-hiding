/* 
 * Disk on RAM Driver 
 * Block driver implementation responsible for 
 * exposing the DOR as the block device files 
 * (dev/rb*) to user-space
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/genhd.h> /* allow partitions and gendisk */
#include <linux/blkdev.h>
#include <linux/errno.h>
#include <linux/blk-mq.h>
#include <linux/hdreg.h>
 
#include "ram_device.h"

#define RB_DEV_NAME "rb"

/* debug messages */
#define debug_args(kern_lvl, fmt, ...) printk(kern_lvl "%s: " fmt, RB_DEV_NAME, ##__VA_ARGS__);
#define debug(kern_lvl, fmt) printk(kern_lvl "%s: " fmt, RB_DEV_NAME);
 
#define RB_FIRST_MINOR 0
/* total number of partitions supported by this device */
#define RB_MINOR_CNT 16
 
static u_int rb_major = 0;
 
/* 
 * The internal structure representation of our Device
 */
static struct rb_device
{
    /* Size is the size of the device (in sectors) */
    unsigned int size;
    /* For exclusive access to our request queue */
    spinlock_t lock;
    /* How many users */
    //short users;
    /* */
    struct blk_mq_tag_set tag_set;	
    /* Our request queue */
    struct request_queue *rb_queue;
    /* This is kernel's representation of an individual disk device */
    struct gendisk *rb_disk;
} rb_dev;
 
static int rb_open(struct block_device *bdev, fmode_t mode)
{
    /* check in which partition we are */
    unsigned unit = iminor(bdev->bd_inode);
 
    debug(KERN_INFO, "Device is opened\n");
    debug_args(KERN_INFO, "Inode number is %d\n", unit);
 
    if (unit > RB_MINOR_CNT)
        return -ENODEV;

    // struct rb_device *dev = bdev->bd_disk->private_data;
    
    // spin_lock(&dev->lock);
    // dev->users++;
    // spin_unlock(&dev->lock);

    return 0;
}
 
static void rb_close(struct gendisk *disk, fmode_t mode)
{
    // struct sbull_dev *dev = disk->private_data;

    // spin_lock(&dev->lock);
	// dev->users--;
    // spin_unlock(&dev->lock);

    debug(KERN_INFO, "Device is closed\n");
}

/*
 * The ioctl() implementation
 */

int rb_ioctl (struct block_device *bdev, fmode_t mode,
                 unsigned int cmd, unsigned long arg)
{
	long size;
	struct hd_geometry geo;
	struct rb_device *dev = bdev->bd_disk->private_data;

	debug(KERN_INFO, ">>> rb_ioctl()\n");

	switch(cmd) {
	    case HDIO_GETGEO:
        /*
		 * Get geometry: since we are a virtual device, we have to make
		 * up something plausible. So we claim 16 sectors, four heads,
		 * and calculate the corresponding number of cylinders.  We set the
		 * start of data at sector four.
		 */
		size = dev->size*(HW_SECTOR_SIZE/RB_SECTOR_SIZE);
		geo.cylinders = (size & ~0x3f) >> 6;
		geo.heads = 4;
		geo.sectors = 16;
		geo.start = 4;
		if (copy_to_user((void __user *) arg, &geo, sizeof(geo)))
			return -EFAULT;
		return 0;
	}

	return -ENOTTY; /* unknown command */
}
 
/* 
 * Actual Data transfer
 * Parses the struct request, which contains the
 * direction of data transfer (read or write),
 * starting sector for the data transfer,
 * total number of sectors for data transfer,
 * and the buffer to put the results on
 */
// static int rb_transfer(struct request *req)
// {
//     //struct rb_device *dev = (struct rb_device *)(req->rq_disk->private_data);
 
//     /* 0 for read, write otherwise */ 
//     int dir = rq_data_dir(req);
//     /* starting sector to process */
//     sector_t start_sector = blk_rq_pos(req);
//     /* amount of sectors to process */
//     //unsigned int sector_cnt = blk_rq_sectors(req);
//     unsigned int sector_cnt = blk_rq_cur_sectors(req);

//     struct bio_vec bv;
//     struct req_iterator iter;
 
//     /* as we iterate, this value is updated so that we
//     process sectors ahead, so to find which sector to 
//     process, we need to sum this to the start_sector */
//     sector_t sector_offset;
//     unsigned int n_sectors;
//     u8 *buffer;

//     /* sector offset in bytes and amount of bytes to process */
//     unsigned long offset_bytes;
//     unsigned long n_bytes;

//     /* so that we can make verifications with its size */
//     struct rb_device *dev = req->rq_disk->private_data;
 
//     int ret = 0;
 
//     //printk(KERN_DEBUG "rb: Dir:%d; Sec:%lld; Cnt:%d\n", dir, start_sector, sector_cnt);
 
//     sector_offset = 0;
//     /*
//      * iterate over the struct request and extract 
//      * information from struct bio_vec (basic input/
//      * output vector) on each iteration
//      */
//     rq_for_each_segment(bv, req, iter)
//     {
//         buffer = page_address(bv.bv_page) + bv.bv_offset;

//         if (bv.bv_len % RB_SECTOR_SIZE != 0)
//         {
//             debug_args(KERN_ERR, "Should never happen: "
//                 "bio size (%d) is not a multiple of RB_SECTOR_SIZE (%d).\n"
//                 "This may lead to data truncation.\n",
//                 bv.bv_len, RB_SECTOR_SIZE);
//             ret = -EIO;
//         }
//         n_sectors = bv.bv_len / RB_SECTOR_SIZE;
//         debug_args(KERN_DEBUG, "Sector Offset: %lld; Buffer: %p; Length: %d sectors\n",
//             sector_offset, buffer, n_sectors);
        
//         /* obtain offset and amount to process, in bytes */
//         offset_bytes = (start_sector + sector_offset) * RB_SECTOR_SIZE;
//         n_bytes = n_sectors * RB_SECTOR_SIZE;

//         /* check if our request is within the limits of the device */
//         if ((offset_bytes + n_bytes) > dev->size) {
//             debug_args(KERN_NOTICE, "Beyond-end write (%ld %ld)\n", offset_bytes, n_bytes);
//             return -EIO;
//         }

//         /* Write to the device */
//         if (dir == WRITE) 
//         {
//             ramdevice_write(offset_bytes, buffer, n_bytes);
//         }
//         /* Read from the device */
//         else 
//         {
//             ramdevice_read(offset_bytes, buffer, n_bytes);
//         }
//         sector_offset += n_sectors;
//     }
//     if (sector_offset != sector_cnt)
//     {
//         debug(KERN_ERR, "bio info doesn't match with the request info");
//         ret = -EIO;
//     }
 
//     return ret;
// }
static int rb_transfer(struct request *req)
{
	//struct rb_device *dev = (struct rb_device *)(req->rq_disk->private_data);

	int dir = rq_data_dir(req);
	sector_t start_sector = blk_rq_pos(req);
	unsigned int sector_cnt = blk_rq_sectors(req);

#define BV_PAGE(bv) ((bv).bv_page)
#define BV_OFFSET(bv) ((bv).bv_offset)
#define BV_LEN(bv) ((bv).bv_len)
	struct bio_vec bv;

	struct req_iterator iter;

	sector_t sector_offset;
	unsigned int sectors;
	u8 *buffer;

	int ret = 0;

	//printk(KERN_DEBUG "rb: Dir:%d; Sec:%lld; Cnt:%d\n", dir, start_sector, sector_cnt);

	sector_offset = 0;
	rq_for_each_segment(bv, req, iter)
	{
		buffer = page_address(BV_PAGE(bv)) + BV_OFFSET(bv);
		if (BV_LEN(bv) % RB_SECTOR_SIZE != 0)
		{
			printk(KERN_ERR "rb: Should never happen: "
				"bio size (%d) is not a multiple of RB_SECTOR_SIZE (%d).\n"
				"This may lead to data truncation.\n",
				BV_LEN(bv), RB_SECTOR_SIZE);
			ret = -EIO;
		}
		sectors = BV_LEN(bv) / RB_SECTOR_SIZE;
		printk(KERN_DEBUG "rb: Start Sector: %llu, Sector Offset: %llu; Buffer: %p; Length: %u sectors\n",
			(unsigned long long)(start_sector), (unsigned long long)(sector_offset), buffer, sectors);
		if (dir == WRITE) /* Write to the device */
		{
			ramdevice_write(start_sector + sector_offset, buffer, sectors);
		}
		else /* Read from the device */
		{
			ramdevice_read(start_sector + sector_offset, buffer, sectors);
		}
		sector_offset += sectors;
	}
	if (sector_offset != sector_cnt)
	{
		printk(KERN_ERR "rb: bio info doesn't match with the request info");
		ret = -EIO;
	}

	return ret;
}
     
/*
 * Represents a block I/O request for us to execute
 * This function should be non blocking, since it's
 * called from a non-process context.
 * This function also takes the queue's spinlock,
 * so it should not be messed with
 */
static blk_status_t rb_request(struct blk_mq_hw_ctx *hctx, const struct blk_mq_queue_data* bd)
{
    struct request *req = bd->rq;
    blk_status_t ret;
 
    blk_mq_start_request (req);

    /* skips non-fs requests */
	if (blk_rq_is_passthrough(req)) {
		debug(KERN_NOTICE, "Skip non-fs request\n");
                ret = BLK_STS_IOERR;  //-EIO
			goto done;
	}
    /* actual data transfer */
    ret = rb_transfer(req);


done:
	blk_mq_end_request (req, ret);
	return ret;
}
 
/* 
 * These are the file operations that performed on the ram block device
 */
static struct block_device_operations rb_fops =
{
    .owner = THIS_MODULE,
    .open = rb_open,
    .release = rb_close,
    .ioctl = rb_ioctl,
};

/* 
 * These are the file operations that performed on the ram block device
 */
static struct blk_mq_ops rb_fops_req =
{
    .queue_rq = rb_request,
};
     
/* 
 * This is the registration and initialization section of the ram block device
 * driver
 */
static int __init rb_init(void)
{
    int ret;
 
    /* Set up our RAM Device */
    if ((ret = ramdevice_init()) < 0)
    {
        return ret;
    }
    rb_dev.size = ret;
 
    /* Get Registered */
    rb_major = register_blkdev(rb_major, RB_DEV_NAME);
    if (rb_major <= 0)
    {
        debug(KERN_ERR, "Unable to get Major Number\n");
        ramdevice_cleanup();
        return -EBUSY;
    }
 
    /* Get a request queue (here queue is created) */
    spin_lock_init(&rb_dev.lock);
    rb_dev.rb_queue = blk_mq_init_sq_queue(&rb_dev.tag_set, &rb_fops_req, 128, BLK_MQ_F_SHOULD_MERGE);
    if (rb_dev.rb_queue == NULL)
    {
        debug(KERN_ERR, "blk_init_queue failure\n");
        unregister_blkdev(rb_major, RB_DEV_NAME);
        ramdevice_cleanup();
        return -ENOMEM;
    }
    /* sets logical block size for the queue */
    blk_queue_logical_block_size(rb_dev.rb_queue, HW_SECTOR_SIZE);
	rb_dev.rb_queue->queuedata = &rb_dev;
     
    /*
     * Add the gendisk structure
     * By using this memory allocation is involved, 
     * the minor number we need to pass bcz the device 
     * will support this much partitions 
     * 
     * The gendisk structure describes the layout of the
     * disk.
     * 
     * The kernel considers this structure as a list of
     * 512 bytes sectors
     */
    rb_dev.rb_disk = alloc_disk(RB_MINOR_CNT);
    if (!rb_dev.rb_disk)
    {
        debug(KERN_ERR, "alloc_disk failure\n");
        blk_cleanup_queue(rb_dev.rb_queue);
        unregister_blkdev(rb_major, RB_DEV_NAME);
        ramdevice_cleanup();
        return -ENOMEM;
    }
 
    /* Setting the major number */
    rb_dev.rb_disk->major = rb_major;
    /* Setting the first mior number */
    rb_dev.rb_disk->first_minor = RB_FIRST_MINOR;
    /* Initializing the device operations */
    rb_dev.rb_disk->fops = &rb_fops;
    /* Driver-specific own internal data */
    rb_dev.rb_disk->private_data = &rb_dev;
    rb_dev.rb_disk->queue = rb_dev.rb_queue;
    /*
     * You do not want partition information to show up in 
     * cat /proc/partitions set this flags
     */
    //rb_dev.rb_disk->flags = GENHD_FL_SUPPRESS_PARTITION_INFO;
    sprintf(rb_dev.rb_disk->disk_name, RB_DEV_NAME);
    /* Setting the capacity of the device in its gendisk structure */
    set_capacity(rb_dev.rb_disk, rb_dev.size);
 
    /* Adding the disk to the system */
    add_disk(rb_dev.rb_disk);
    /* Now the disk is "live" */
    debug_args(KERN_INFO, "Ram Block driver initialised (%d sectors; %d bytes)\n",
        rb_dev.size, rb_dev.size * RB_SECTOR_SIZE);

    debug(KERN_INFO, "Ram Block driver initialised\n");
 
    return 0;
}
/*
 * This is the unregistration and uninitialization section of the ram block
 * device driver
 */
static void __exit rb_cleanup(void)
{
    del_gendisk(rb_dev.rb_disk);
    put_disk(rb_dev.rb_disk);
    blk_cleanup_queue(rb_dev.rb_queue);
    unregister_blkdev(rb_major, RB_DEV_NAME);
    ramdevice_cleanup();
}
 
module_init(rb_init);
module_exit(rb_cleanup);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Anil Kumar Pugalia <email@sarika-pugs.com>");
MODULE_DESCRIPTION("Ram Block Driver");
MODULE_ALIAS_BLOCKDEV_MAJOR(rb_major);