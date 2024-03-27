/*
 * This block driver is from the Linux Device Drivers 3rd edition
 * chapter 16 (everything is explained in there): 
 * https://static.lwn.net/images/pdf/LDD3/ch16.pdf
 * 
 * sbull_ram.c - implement a set of in-memory virtual
 *          disk drives. For each drive, sbull allocates
 *          an array of memory and makes that array available
 *          via block operations
 *          This code implements a simple RAM-based disk device
 * 
 * It is not a realistic implementation of a driver for devices 
 * because:
 *      1. subull executes requests synchronously, one request at 
 *      each time
 *      2. does not try to find adjacent requests to perform larger
 *      operations, in order to transfer as much data as possible in 
 *      a single request. Here, only one buffer is transfered each time
 * 
 * Can be tested via partitioning the virtual device, building 
 * file systems on it and mounting it in the system hierarchy
 * 
 */

#include <linux/version.h> 	/* LINUX_VERSION_CODE */
#include <linux/module.h>
#include <linux/kernel.h>   /* printk() */
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>
#include <linux/timer.h>
#include <linux/slab.h>		/* kmalloc() */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Daniela Lopes");

/*
 * -----------------------------------------------------
 *              Initializations
 * -----------------------------------------------------
 */

/* 
 * Number of minor devices each sbull device supports
 * sbulla -> first minor number
 * sbullb -> second minor number
 *      third partition of second device is in: /dev/sbullb3
 * ...
 */
#define SBULL_MINORS 1

/*
 * We can tweak our hardware sector size, but the kernel talks to us
 * in terms of small sectors, always
 */
#define KERNEL_SECTOR_SIZE	512


static int sbull_major = 0;
// var name, var type and permissions
//module_param(sbull_major, int, 0);
static int hardsect_size = 512;
//module_param(hardsect_size, int, 0);
static int nsectors = 1024;	/* How big the drive is */
//module_param(nsectors, int, 0);
//static int ndevices = 4;
static int ndevices = 1;

/*
 * The different "request modes" we can use.
 */
//  {
// 	enumRM_SIMPLE  = 0,	/* The extra-simple request function */
// 	RM_FULL    = 1,	/* The full-blown version */
// 	RM_NOQUEUE = 2,	/* Use make_request */
// };
// static int request_mode = RM_SIMPLE;

/* 
 * Description of our device structure
 */ 
struct sbull_dev {
    int size; /* Device size in sectors */
    u8 *data; /* The data array */
    short users; /* How many users */
    //short media_change; /* Flag a media change? */
    spinlock_t lock; /* For mutual exclusion */
    struct blk_mq_tag_set tag_set;
    struct request_queue *queue; /* The device request queue */
    struct gendisk *gd; /* The gendisk structure */
    //struct timer_list timer; /* For simulated media changes */
};

static struct sbull_dev *dev = NULL;


/*
 * -----------------------------------------------------
 *          Block operations declaration
 * -----------------------------------------------------
 */
static int sbull_open(struct block_device *b_dev, fmode_t mode);

static void sbull_release(struct gendisk *disk, fmode_t mode);

//int sbull_media_changed(struct gendisk *gd);

//int sbull_revalidate(struct gendisk *gd);

int sbull_ioctl (struct block_device *b_dev, fmode_t mode, unsigned cmd, unsigned long arg);

static blk_status_t sbull_request(struct blk_mq_hw_ctx *hctx, const struct blk_mq_queue_data* bd);

static void sbull_transfer(struct sbull_dev *dev, unsigned long sector,
                            unsigned long nsect, char *buffer, int write);


/*
 * -----------------------------------------------------
 *          Declaration of operations 
 *          structures for block devices
 * -----------------------------------------------------
 */

static struct blk_mq_ops mq_ops = {
    .queue_rq = sbull_request,
};

/*
 * The device operations structure
 */
static struct block_device_operations sbull_ops = {
	.owner           = THIS_MODULE,
	.open 	         = sbull_open,
	.release 	     = sbull_release,
	//.media_changed   = sbull_media_changed,
	//.revalidate_disk = sbull_revalidate,
	.ioctl	         = sbull_ioctl
};


/*
 * -----------------------------------------------------
 *          Block operations implementation
 * -----------------------------------------------------
 */

/*
 * Used to maintain a count of users of the block device
 * Increases the counter
 * 
 * Example of operations that open a block device:
 * partitioning a disk, building a file system on a partition,
 * or running a filesystem checker or when a partition is mounted
 */
static int sbull_open(struct block_device *b_dev, fmode_t mode)
{   
    struct sbull_dev *dev_in_use = b_dev->bd_disk->private_data;
    //filp->private_data = dev_in_use;

    // acquire lock to make changes to ...
    spin_lock(&dev_in_use->lock);

    if (!dev_in_use->users) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
		check_disk_change(b_dev);
#else
        /* For newer kernels (as of 5.10), bdev_check_media_change()
        * is used, in favor of check_disk_change(),
        * with the modification that invalidation
        * is no longer forced. */
        if(bdev_check_media_change(b_dev)) {
            struct gendisk *gd = b_dev->bd_disk;
            const struct block_device_operations *bdo = gd->fops;
            if (bdo && bdo->revalidate_disk)
                bdo->revalidate_disk(gd);
        }
#endif
    }
    dev_in_use->users++;
    spin_unlock(&dev_in_use->lock);

    printk(">>> sbull_open()\n");
    
    return 0;
}

/*
 * Used to maintain a count of users of the block device
 * Decrements the counter
 */
static void sbull_release(struct gendisk *disk, fmode_t mode)
{
    struct sbull_dev *dev_in_use= disk->private_data;
    spin_lock(&dev_in_use->lock);
    dev_in_use->users--;
    /*if (!dev->users) {
        dev->timer.expires = jiffies + INVALIDATE_DELAY;
        add_timer(&dev->timer);
    }*/
    spin_unlock(&dev_in_use->lock);

    printk(">>> sbull_release()\n");
}

/*
 * Removable media support
 * Should return non zero balue if media has changed
 * 
 * For sbull, we just use a flag that is set if the media removal
 * timer has expired (30 seconds without usage) 
 */
/*int sbull_media_changed(struct gendisk *gd)
{
    struct sbull_dev *dev = gd->private_data;
    return dev->media_change;
}*/

/*
 * Removable media support
 * Is called after a media change
 * Does whatever is necessary to prepare the driver for operations
 * on the new media, if any
 * After this is called, the kernel attempts to reread the partition
 * table and start over with the device
 * 
 * For sbull, we simply reset the media_change flg and zeroes out the
 * device memory to simulate the insertion of a blank disk
 */
/*int sbull_revalidate(struct gendisk *gd)
{
    struct sbull_dev *dev = gd->private_data;
    if (dev->media_change) {
        dev->media_change = 0;
        memset (dev->data, 0, dev->size);
    }
    return 0;
}*/

/*
 * These are commands that the higher-level block subsystem
 * intercepts before the driver sees them
 */
int sbull_ioctl(struct block_device *b_dev, fmode_t mode, 
                unsigned cmd, unsigned long arg)
{
    printk(">>> sbull_ioctl()\n");
    return -ENOTTY; /* unknown command */
}

/*
 * NOTE: needs to be defined, otherwise it'll block forever
 * whenever the block device is accessed, which happens right
 * during initialization, for instance in set_capacity()
 * 
 * It does not actually need to complete all of the requests
 * on the queue before it returns. But it must make a start
 * on those requests and ensure that they are eventually processed
 * by the driver
 * 
 * This function is associated with the request_queue when that queue
 * is created
 * 
 * It also holds the lock, but it can free the lock if it is not going to 
 * access any data structure anymore
 * 
 * Generally, it is executed asynchronously
 * 
 * We do not know if the I/O buffer provided by the request is in kernel
 * or user space. Any sort of operation that explicitly accesses user space
 * is in error and will certainly lead to trouble
 * 
 * This function is called once per request, and only iterates through the data
 * of that single request
 */
static blk_status_t sbull_request(struct blk_mq_hw_ctx *hctx, const struct blk_mq_queue_data* bd)
{   
    /*
     * Here we are going to pick the next request to process on the blk_mq_queue_data
     * struct blk_mq_queue_data {
     *      struct request * rq;
     *      bool last;
     * };
     */
    struct request *req = bd->rq;
    /*
     * req->rq_disk is the gendisk for this request
     * req->rq_disk->private_data is equivalent to the structure we defined 
     *                              for our device
     */
	struct sbull_dev *dev = req->rq_disk->private_data;
    /*
     * the bio_vec points to the pages with the request data
     */
    struct bio_vec bvec;
    /*
     * structure required to iterate through the bio_vec
     */
    struct req_iterator iter;
    /*
     * Obtains the sector in which the request starts
     */
    sector_t pos_sector = blk_rq_pos(req);
    printk (KERN_INFO ">>> Request starts on sector %ld\n", pos_sector);
	void *buffer;
 	blk_status_t ret;

    /*
     * Notify the kernel that we are going to start processing this request
     */
	blk_mq_start_request(req);

    /*
     * Checks if the request is non-fs. If it is, the request is discarded by
     * this function
     */
	if (blk_rq_is_passthrough(req)) {
		printk (KERN_NOTICE "Skip non-fs request\n");
        ret = BLK_STS_IOERR;  //-EIO
			goto done;
	}
    /*
     * Iterates through the bio and corresponding bio_vec structures,
     * which allows to get the pages with the request data
     */
	rq_for_each_segment(bvec, req, iter);
	{
        /* Check in which sector of the disk this segment is in */
		size_t num_sector = blk_rq_cur_sectors(req);
		printk (KERN_NOTICE "Req dev %u dir %d sec %lld, nr %ld\n",
                        //(unsigned)(dev - Devices), rq_data_dir(req),
                        (unsigned)(dev), rq_data_dir(req),
                        pos_sector, num_sector);
        /* print if it's a read or a write */
        if (rq_data_dir(req) == WRITE) printk (KERN_INFO ">>> Processing WRITE ...\n");
        else printk (KERN_INFO ">>> Processing READ ...\n");

        /*
         * Gets the address of the page we want to read from
         */
		buffer = page_address(bvec.bv_page) + bvec.bv_offset;
		/* 
         * this function is responsible for copying data between the request pages
         * and the internal buffer
         */
        sbull_transfer(dev, pos_sector, num_sector,
				buffer, rq_data_dir(req) == WRITE);
        /*
         * Updates the sectors of the request, remembering the same request does not 
         * target non-contiguous blocks
         */
		pos_sector += num_sector;
	}
	ret = BLK_STS_OK;
done:
    /*
     * Notify the kernel that we finished processing this request
     * This is a wrapper for blk_update_request() + __blk_mq_end_request()
     */
	blk_mq_end_request (req, ret);

    printk(">>> sbull_request()\n");

	return ret;
}

/*
 * Responsible for transfering data between the pages of the request and our device
 * buffer
 */
static void sbull_transfer(struct sbull_dev *dev, unsigned long sector,
                            unsigned long nsect, char *buffer, int write)
{
    /* offset represents the place in the buffer we want to start reading from
     or writing to */
    unsigned long offset = sector*KERNEL_SECTOR_SIZE;
    /* nbytes represents the amount of bytes we are supposed to read from or
    to write to our block device */
    unsigned long nbytes = nsect*KERNEL_SECTOR_SIZE;
    printk(KERN_INFO "write to or read from (%ld %ld)\n", offset, nbytes);
    /* 
     * handle scaling of vector sizes and ensures that we do not try to copy
     * beyond the end of the virtual device
     */
    if ((offset + nbytes) > dev->size) {
        printk(KERN_NOTICE "Beyond-end write (%ld %ld)\n", offset, nbytes);
        return;
    }
    /* Data transfer can be done with memcpy because the data is already in memory */
    if (write)
        memcpy(dev->data + offset, buffer, nbytes);
    else
        memcpy(buffer, dev->data + offset, nbytes);

    printk(">>> sbull_transfer()\n");
}


/*
 * -----------------------------------------------------
 *          Module init and exit functions
 * -----------------------------------------------------
 */
static int __init sbull_init(void)
{
    //int i;
	
    /*
     * ------------------------------------------------------------
     * Allocates major and creates the entry /proc/devices/sbull
     * ------------------------------------------------------------
     */
    sbull_major = register_blkdev(sbull_major, "sbull");
    if (sbull_major <= 0) {
        printk(KERN_WARNING "sbull: unable to get major number\n");
        return -EBUSY;
    } 

    printk(">>> register_block_device()\n");

	/*
     * ------------------------------------------------------------
	 * Allocate the device array, and initialize each one.
     * ------------------------------------------------------------
	 */
	dev = kmalloc(ndevices*sizeof (struct sbull_dev), GFP_KERNEL);
	if (dev == NULL) {
		unregister_blkdev(sbull_major, "sbull");
	    return -ENOMEM;
    }
    
    /*
     * ------------------------------------------------------------
     * Allocation of memory buffers to be used by block operations
     * ------------------------------------------------------------
     */
    //for (i = 0; i < ndevices; i++) 
		//setup_device(Devices + i, i);
    memset (dev, 0, sizeof(struct sbull_dev));
    dev->size = nsectors*hardsect_size;
    dev->data = vmalloc(dev->size);
    if (dev->data == NULL) {
        printk (KERN_NOTICE "vmalloc failure.\n");
        return;
    }

    printk(">>> allocated memory\n");

    /*
     * ------------------------------------------------------------
     * Allocate and initialize a spinlock and a request_queue
     * sbull request() is our implementation of block read and 
     * write requests.
     * 
     * When we allocate a request_queue, we should pass to it a 
     * spinlock that controls the access to it
     * ------------------------------------------------------------
     */
    spin_lock_init(&dev->lock);

    /*
	 * The I/O queue, depending on whether we are using our own
	 * make_request function or not.
	 */
// 	switch (request_mode) {
// 	    case RM_NOQUEUE:
// #if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0))
// 		    dev->queue =  blk_generic_alloc_queue(sbull_make_request, NUMA_NO_NODE);
// #else
// 		    dev->queue =  blk_generic_alloc_queue(NUMA_NO_NODE);
// #endif
//             if (dev->queue == NULL)
//                 goto out_vfree;
//             break;

// 	    case RM_FULL:
//             //dev->queue = blk_init_queue(sbull_full_request, &dev->lock);
//             dev->queue = blk_mq_init_sq_queue(&dev->tag_set, &mq_ops_full, 128, BLK_MQ_F_SHOULD_MERGE);
//             if (dev->queue == NULL)
//                 goto out_vfree;
//             break;

// 	    default:
// 		    printk(KERN_NOTICE "Bad request mode %d, using simple\n", request_mode);
//         	/* fall into.. */
	
// 	    case RM_SIMPLE:
            //dev->queue = blk_init_queue(sbull_request, &dev->lock);
        //dev->queue = blk_mq_init_sq_queue(&dev->tag_set, &mq_ops_simple, 128, BLK_MQ_F_SHOULD_MERGE);
        dev->queue = blk_mq_init_sq_queue(&dev->tag_set, &mq_ops, 128, BLK_MQ_F_SHOULD_MERGE);
        if (dev->queue == NULL)
            goto out_vfree;
        //break;
	//}
	blk_queue_logical_block_size(dev->queue, hardsect_size);
	dev->queue->queuedata = dev;

    printk(">>> initialized queue and lock\n");

    /*
     * ------------------------------------------------------------
     * Allocate, initialize and install gendisk
     * ------------------------------------------------------------
     */
    int dev_num = 0;

    dev->gd = alloc_disk(SBULL_MINORS);
    if (!dev->gd) {
        printk (KERN_NOTICE "alloc_disk failure\n");
        if (dev->data)
		    vfree(dev->data);
    }
    printk(">> if (!dev->gd) {\n");
    dev->gd->major = sbull_major;
    dev->gd->first_minor = 0;
    dev->gd->fops = &sbull_ops;
    dev->gd->queue = dev->queue;
    dev->gd->private_data = dev;
    printk(">> dev->gd->private_data = dev\n");
    /* Set device name as it will be represented in /dev */
    snprintf (dev->gd->disk_name, 32, "sbull%c", dev_num + 'a');
    set_capacity(dev->gd, nsectors*(hardsect_size/KERNEL_SECTOR_SIZE));
    
    printk(">> after set_capacity\n");

    /*
     * ------------------------------------------------------------
     * Last thing that must be done is call add_disk()
     * ------------------------------------------------------------
     */
    /* Finally creates /dev/sbull file and activates the driver to use*/
    add_disk(dev->gd);

    printk(">>> initialized gendisk\n");

    printk(">>> sbull_init()\n");
	
    return 0;

out_vfree:
	if (dev->data)
		vfree(dev->data);
}

static void __exit sbull_exit(void)
{   
    //int i;

	//for (i = 0; i < ndevices; i++) {
		//struct sbull_dev *dev = Devices + i;

		/*del_timer_sync(&dev->timer);
		if (dev->gd) {
			del_gendisk(dev->gd);
			put_disk(dev->gd);
		}
		if (dev->queue) {
			if (request_mode == RM_NOQUEUE)
				//kobject_put (&dev->queue->kobj);
				blk_put_queue(dev->queue);
			else
				blk_cleanup_queue(dev->queue);
		}
		if (dev->data)
			vfree(dev->data);
	}*/

    //struct sbull_dev *dev = Devices + i;
    /*
     * ------------------------------------------------------------
     * Deallocation of memory buffers
     * ------------------------------------------------------------
     */
    if (dev->gd) {
        del_gendisk(dev->gd);
        put_disk(dev->gd);
    }
    if (dev->queue) {
        //if (request_mode == RM_NOQUEUE)
            //kobject_put (&dev->queue->kobj);
            //blk_put_queue(dev->queue);
        //else
        blk_cleanup_queue(dev->queue);
    }
    if (dev->data)
        vfree(dev->data);

    printk(">>> deallocated memory\n");

    unregister_blkdev(sbull_major, "sbull");
	kfree(dev);

	printk(">>> sbull_exit()\n");
}

module_init(sbull_init);
module_exit(sbull_exit);