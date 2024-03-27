#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>

#define MY_BLOCK_MAJOR           240
#define MY_BLKDEV_NAME          "mybdev"

#define MY_BLOCK_MINORS       1

#define NR_SECTORS                   1024
#define KERNEL_SECTOR_SIZE           512

/*
 * ----------------------------------------------
 *                  Initializations
 * ----------------------------------------------
 */

/*
 * to store important elements describing the block device
 */
static struct my_block_dev {
    spinlock_t lock;                /* For mutual exclusion */
    struct request_queue *queue;    /* The device request queue */
    struct gendisk *gd;             /* The gendisk structure */
    //...
} dev;


/*
 * structure to reimplement block_device_operations
 */
struct block_device_operations my_block_ops = {
    .owner = THIS_MODULE,
    .open = my_block_open,
    .release = my_block_release
};


/*
 * ----------------------------------------------
 *      block_device_operations implementation
 * ----------------------------------------------
 */

/*
 * there are no read or write operations because these are performed by
 * the request() function associated with the request queue of the disk
 */

static int my_block_open(struct block_device *bdev, fmode_t mode)
{
    //...

    return 0;
}

static int my_block_release(struct gendisk *gd, fmode_t mode)
{
    //...

    return 0;
}


/*
 * ----------------------------------------------
 *          request queue functions
 * ----------------------------------------------
 */
static void my_block_request(struct request_queue *q) {
    struct request *rq;
    struct my_block_dev *dev = q->queuedata;

    /*
     * main loop to interate through the request queue sent as argument
     */
    while (1) {
        // read first request from the queue and starts the request
        rq = blk_fetch_request(q);
        // if NULL, there's no request left to be processed and exits this function
        if (rq == NULL)
            break;

        // receive calls that do not transfer data blocks return an error because the driver
        // does not know how to handle them
        if (blk_rq_is_passthrough(rq)) {
            printk (KERN_NOTICE "Skip non-fs request\n");
            __blk_end_request_all(rq, -EIO);
           continue;
        }

        /* do work */

        __blk_end_request_all(rq, 0);
    }
}


/*
 * ----------------------------------------------
 *                      Init module
 * ----------------------------------------------
 */
static int create_block_device(struct my_block_dev *dev)
{
    /* Initialize the I/O queue */
    spin_lock_init(&dev->lock);
    /*
     * my_block_request is the pointer to the function which processes requests for the device
     * spinlock is held by the kernel during request() to ensure exclusive access to the queue
     */
    dev->queue = blk_init_queue(my_block_request, &dev->lock);
    if (dev->queue == NULL)
        goto out_err;
    blk_queue_logical_block_size(dev->queue, KERNEL_SECTOR_SIZE);
    /* equivalent to the private_data field */
    dev->queue->queuedata = dev;

    out_err:
        return -ENOMEM;

    // structure gendisk to work with block devices
    dev->gd = alloc_disk(MY_BLOCK_MINORS);
    if (!dev->gd) {
        printk (KERN_NOTICE "alloc_disk failure\n");
        return -ENOMEM;
    }

    dev->gd->major = MY_BLOCK_MAJOR;
    dev->gd->first_minor = 0;
    dev->gd->fops = &my_block_ops;
    dev->gd->queue = dev->queue;
    dev->gd->private_data = dev;
    snprintf (dev->gd->disk_name, 32, "myblock");
    set_capacity(dev->gd, NR_SECTORS); // conversion between kernel amount of data and 512 transfered between device and driver

    add_disk(dev->gd); // should only be called after driver is fully initialized

    return 0;
}

static int my_block_init(void)
{
    int status;

    status = register_blkdev(MY_BLOCK_MAJOR, MY_BLKDEV_NAME);
    if (status < 0) {
        printk(KERN_ERR "unable to register mybdev block device\n");
        //return -EBUSY;
        return status;
     }
     //...
     create_block_device(&dev);

     return 0;
}


/*
 * ----------------------------------------------
 *                      Exit module
 * ----------------------------------------------
 */
static void delete_block_device(struct my_block_dev *dev)
{
    // keep number of users of the device and call del_gendisk() when there are no more users for that device
    if (dev->gd)
        del_gendisk(dev->gd);
    //...
    if (dev->queue)
        blk_cleanup_queue(dev->queue);
}

static void my_block_exit(void)
{
    delete_block_device(&dev);
    //...
    unregister_blkdev(MY_BLOCK_MAJOR, MY_BLKDEV_NAME);
}