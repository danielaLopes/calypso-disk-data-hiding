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

 
#define DEV_NAME "loop_snooper"


static int calypso_dev_open(struct block_device *bdev, fmode_t mode);

static void calypso_dev_close(struct gendisk *disk, fmode_t mode);

int calypso_dev_ioctl(struct block_device *bdev, fmode_t mode, unsigned cmd, unsigned long arg);

static blk_qc_t calypso_make_request(struct request_queue *q, struct bio *bio);
//static blk_qc_t calypso_make_request(struct bio *bio);

static struct calypso_blk_device *calypso_dev = NULL;

/* pointer to physical device structure */
static struct block_device *physical_dev;

static int major = 0;

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
	debug_args(KERN_INFO, __func__, "Remapping requests from calypso device to %s", PHYSICAL_DISK_NAME);

	bio_set_dev(bio, calypso_dev->physical_dev);

	/* map request from calypso logical device to the physical device */
    trace_block_bio_remap(bdev_get_queue(calypso_dev->physical_dev), bio,
            bio_dev(bio), bio->bi_iter.bi_sector);

    /* No need to call bio_endio() */
    generic_make_request(bio);
	//submit_bio_noacct(bio);
}

/* 
 * Snoop requests to physical device
 */
static int calypso_snoop_io_requests_thread_fn(void *data)
{
    struct request_queue *physical_dev_queue = bdev_get_queue(calypso_dev->physical_dev);

	int n_requests = 0;

	while (!kthread_should_stop()) 
	{	
		debug(KERN_INFO, __func__, "Thread is inside while");

		// wait_event_interruptible(req_event, kthread_should_stop() ||
        //     physical_dev_queue->last_merge);
		//wait_event_interruptible(req_event, kthread_should_stop());

		// while(!physical_dev_queue->last_merge) {

		// }

        /*
         * Checks if the physical device received requests every 500 jiffies
         */
		wait_event_timeout(req_event, kthread_should_stop() ||
            physical_dev_queue->last_merge, 500);

		struct request *next_request = physical_dev_queue->last_merge;
		// if (next_request)
		// {
			n_requests++;
			debug_args(KERN_INFO, __func__, "%s received %d requests", PHYSICAL_DISK_NAME, n_requests);
		// }
	}

	return 0;
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
	remap_io_request_to_physical_dev(bio);

    return 0;

}

// TODO: make these call the original functions
static int calypso_dev_open(struct block_device *bdev, fmode_t mode)
{
    debug(KERN_INFO, __func__, "Device is opened\n");

    return 0;
}

static void calypso_dev_close(struct gendisk *disk, fmode_t mode)
{
    debug(KERN_INFO, __func__, "Device is closed\n");
}

int calypso_dev_ioctl(struct block_device *bdev, fmode_t mode, 
						unsigned cmd, unsigned long arg)
{
    debug_args(KERN_INFO, __func__, "ioctl call, cmd: %d\n", cmd);

    return -ENOTTY;
}

/* 
 * This is the registration and initialization section of the block device
 * driver
 */
static int __init calypso_init(void)
{
    int ret = 0;
	
	debug(KERN_INFO, __func__, "Initializing Calypso\n");

	ret = calypso_dev_init(&calypso_dev);
	if (ret != 0)
    {
        debug(KERN_ERR, __func__, "Unable to initialize Calypso\n");
		return ret;
    } else {
		debug(KERN_INFO, __func__, "Calypso driver is initialized\n");
	}

	calypso_dev->capacity = CALYPSO_DEVICE_SIZE;

    //ret = calypso_dev_setup(calypso_dev, &calypso_fops, &calypso_fops_req);
	ret = calypso_dev_no_queue_setup(calypso_dev, &calypso_fops, DEV_NAME, &major, calypso_make_request);
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

	calypso_dev->snooping_thread = kthread_create(calypso_snoop_io_requests_thread_fn, 
											NULL,
           									"calypso_snooping_thread");
    if (IS_ERR(calypso_dev->snooping_thread))
    {
        debug_args(KERN_ERR, __func__, "kthread_create <%lu>\n",
               PTR_ERR(calypso_dev->snooping_thread));
		return -EFAULT;
    }

	wake_up_process(calypso_dev->snooping_thread);

	wake_up(&req_event);

    return ret;
}

/*
 * This is the unregistration and uninitialization section of the ram block
 * device driver
 */
static void __exit calypso_cleanup(void)
{
	kthread_stop(calypso_dev->snooping_thread);

    calypso_close_physical_disk(physical_dev);

	calypso_dev_physical_cleanup(calypso_dev, DEV_NAME, &major);
	
    debug(KERN_DEBUG, __func__, "Calypso driver cleanup");
}
 
module_init(calypso_init);
module_exit(calypso_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Daniela Lopes");
MODULE_DESCRIPTION("Calypso Driver");