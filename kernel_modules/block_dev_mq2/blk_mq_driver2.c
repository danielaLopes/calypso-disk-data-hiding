/*
 * blk_mq_driver.c - 
 * 
 * https://github.com/CodeImp/sblkdev
 * 
 * Usage:
 *       $ sudo fdisk /dev/blkdev0
 *       $ o
 *       $ w
 *       $ sudo fdisk /dev/blkdev0
 *       $ p
 *       $ sudo fdisk -l 
 * 
 *      Read: 
 *          $ sudo cat /dev/blkdev0
 *          
 *          [117211.641565] sblkdev: request start from sector 0
            [117211.641566] sblkdev: request process 4096 bytes
            [117211.641580] sblkdev: process_requests()
            [117211.641580] sblkdev: do_simple_request()
            [117211.641580] sblkdev: request start from sector 8
            [117211.641581] sblkdev: request process 4096 bytes
            [117211.641612] sblkdev: process_requests()
            [117211.641613] sblkdev: do_simple_request()
            [117211.641613] sblkdev: request start from sector 32
            [117211.641614] sblkdev: request process 4096 bytes
            [117211.641619] sblkdev: process_requests()
            [117211.641619] sblkdev: do_simple_request()
            [117211.641619] sblkdev: request start from sector 64
            [117211.641620] sblkdev: request process 4096 bytes
            [117211.641631] sblkdev: process_requests()
            [117211.641631] sblkdev: do_simple_request()
            [117211.641632] sblkdev: request start from sector 24
            [117211.641632] sblkdev: request process 4096 bytes
            [117211.641636] sblkdev: process_requests()
            [117211.641637] sblkdev: do_simple_request()
            [117211.641637] sblkdev: request start from sector 56
            [117211.641638] sblkdev: request process 4096 bytes
            [117211.641644] sblkdev: process_requests()
            [117211.641644] sblkdev: do_simple_request()
            [117211.641645] sblkdev: request start from sector 120
            [117211.641645] sblkdev: request process 4096 bytes
            [117211.641658] sblkdev: process_requests()
            [117211.641658] sblkdev: do_simple_request()
            [117211.641659] sblkdev: request start from sector 16
            [117211.641660] sblkdev: request process 4096 bytes
            [117590.252894] sblkdev: _release()
            [117616.491695] sblkdev: _open()
            [117616.491733] sblkdev: process_requests()
            [117616.491734] sblkdev: do_simple_request()
            [117616.491735] sblkdev: request start from sector 0
            [117616.491741] sblkdev: request process 65536 bytes
            [117616.497054] sblkdev: _release()

        Write:
            $ sudo su
            $ echo "hello world" > /dev/blkdev0
            $ cat /dev/blkdev0
            > helloworld
            $ exit

        For instance:
            Create new partition:
                $ sudo fdisk /dev/blkdev0
                $ p
                $ e
                First sector (1-127, default 1): 
                $ 1
                Last sector, +/-sectors or +/-size{K,M,G,T,P} (1-127, default 127): 120
                $ 120
                Created a new partition 1 of type 'Extended' and of size 60 KiB (from sector 1 to sector 120)
                > Now within this new extended partition we can create more logical partitions
                $ p
                Device         Boot Start   End Sectors Size Id Type
                /dev/blkdev0p1          1   120     120  60K  5 Extended
                $ n
                First sector (2-120, default 2): 
                $ 2
                Last sector, +/-sectors or +/-size{K,M,G,T,P} (2-120, default 120): 
                $ 100

                Created a new partition 5 of type 'Linux' and of size 49,5 KiB.
                $ p
                Device         Boot Start   End Sectors  Size Id Type
                /dev/blkdev0p1          1   120     120   60K  5 Extended
                /dev/blkdev0p5          2   100      99 49,5K 83 Linux
                $ n
                Partition type
                p   primary (0 primary, 1 extended, 3 free)
                l   logical (numbered from 5)
                $ l

                Adding logical partition 6
                First sector (102-120, default 102): 
                Last sector, +/-sectors or +/-size{K,M,G,T,P} (102-120, default 120): 
                $ 110

                Created a new partition 6 of type 'Linux' and of size 4,5 KiB.

                $ n
                Partition type
                p   primary (0 primary, 1 extended, 3 free)
                l   logical (numbered from 5)
                $ p
                Partition number (2-4, default 2): 
                First sector (121-127, default 121): 
                $ 121
                Last sector, +/-sectors or +/-size{K,M,G,T,P} (121-127, default 127): 

                Created a new partition 2 of type 'Linux' and of size 3,5 KiB.
                $ p
                Disk /dev/blkdev0: 64 KiB, 65536 bytes, 128 sectors
                Units: sectors of 1 * 512 = 512 bytes
                Sector size (logical/physical): 512 bytes / 512 bytes
                I/O size (minimum/optimal): 512 bytes / 512 bytes
                Disklabel type: dos
                Disk identifier: 0xb10dbc39

                Device         Boot Start   End Sectors  Size Id Type
                /dev/blkdev0p1          1   120     120   60K  5 Extended
                /dev/blkdev0p2        121   127       7  3,5K 83 Linux
                /dev/blkdev0p5          2   100      99 49,5K 83 Linux
                /dev/blkdev0p6        102   110       9  4,5K 83 Linux

                Partition table entries are not in disk order.
                $ n
                All space for primary partitions is in use.
                Adding logical partition 7
                First sector (112-120, default 112): 
                Last sector, +/-sectors or +/-size{K,M,G,T,P} (112-120, default 120): 

                Created a new partition 7 of type 'Linux' and of size 4,5 KiB.
 * 
 * 
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/buffer_head.h>
#include <linux/blk-mq.h>
#include <linux/hdreg.h>

#ifndef SUCCESS
#define SUCCESS 0
#endif

/*
 * -----------------------------------------------------
 *          Block operations declaration
 * -----------------------------------------------------
 */

static int _open(struct block_device *dev, fmode_t mode);

static void _release(struct gendisk *gdisk, fmode_t mode);

int _ioctl(struct block_device *bdev, fmode_t mode, unsigned cmd, unsigned long arg);

static blk_status_t process_requests(struct blk_mq_hw_ctx * hctx, const struct blk_mq_queue_data * bd);


typedef struct sblkdev_cmd_s
{
    //nothing
} sblkdev_cmd_t;

// The internal representation of our device
typedef struct sblkdev_device_s
{
    sector_t capacity; // Device size in bytes
    u8 * data; // The data aray. u8 - 8 bytes
    atomic_t open_counter; // How many openers

    struct blk_mq_tag_set tag_set;
    struct request_queue * queue; // For mutual exclusion

    struct gendisk * disk; // The gendisk structure
} sblkdev_device_t;

static struct blk_mq_ops _mq_ops = {
    .queue_rq = process_requests, /* function that handles processing of requests */
};

static const struct block_device_operations _fops = {
    .owner = THIS_MODULE,
    .open = _open,
    .release = _release,
    .ioctl = _ioctl,
};

static const char * _sblkdev_name = "blkdev";
static const size_t _sblkdev_buffer_size = 16 * PAGE_SIZE;

static int _sblkdev_major = 0;
static sblkdev_device_t * _sblkdev_device = NULL;


/*
 * -----------------------------------------------------
 *          Block operations implementation
 * -----------------------------------------------------
 */

static int _open(struct block_device *dev, fmode_t mode)
{
    printk (KERN_INFO "sblkdev: _open()\n");

    return SUCCESS;
}

static void _release(struct gendisk *gdisk, fmode_t mode)
{
    printk (KERN_INFO "sblkdev: _release()\n");
}

int _ioctl(struct block_device *bdev, fmode_t mode, unsigned cmd, unsigned long arg)
{
    printk (KERN_INFO "sblkdev: _ioctl()\n");

    return -ENOTTY;
}


/*
 * -----------------------------------------------------
 *                 Processing requests
 * -----------------------------------------------------
 */

/*
 * Function that handles the transfer of bytes from the request to the buffer and back
 */
static int do_simple_request (struct request * rq, unsigned int * nr_bytes)
{
    int ret = SUCCESS;
    struct bio_vec bvec;
    struct req_iterator iter;
    sblkdev_device_t * dev = rq-> q-> queuedata;
    loff_t pos = blk_rq_pos (rq) << SECTOR_SHIFT;
    loff_t dev_size = (loff_t)(dev->capacity << SECTOR_SHIFT);

    printk (KERN_INFO "sblkdev: do_simple_request()\n");

    printk(KERN_WARNING "sblkdev: request start from sector %llu\n", blk_rq_pos(rq));
    
    /* goes through all the bio, allowing us to get to the pages with the request data
    Processing the request is generally just copying the data between the request pages and
    the internal buffer */
    rq_for_each_segment(bvec, rq, iter)
    {
        unsigned long b_len = bvec.bv_len;

        void* b_buf = page_address(bvec.bv_page) + bvec.bv_offset;

        if ((pos + b_len) > dev_size)
            b_len = (unsigned long) (dev_size - pos);

        if (rq_data_dir (rq)) // WRITE
            memcpy (dev->data + pos, b_buf, b_len);
        else // READ
            memcpy (b_buf, dev->data + pos, b_len);

        pos += b_len;
        *nr_bytes += b_len;
    }

    return ret;
}

/*
 * Implementation of the function to process requests
 * 
 * hctx is the state of the hardware queue. In our case, we
 * don't use the hardware queue, so it's unused
 * 
 * bd
 *      struct blk_mq_queue_data {
 *          struct request * rq;
 *          bool last;
 *      };
 * 
 * This is not for real use since we can't use regular synchronization here! 
 */
static blk_status_t process_requests(struct blk_mq_hw_ctx * hctx, const struct blk_mq_queue_data * bd)
{
    blk_status_t status = BLK_STS_OK;
    struct request * rq = bd->rq;

    printk (KERN_INFO "sblkdev: process_requests()\n");

    blk_mq_start_request (rq);

    // we can't use that thread
    {
        unsigned int nr_bytes = 0;

        if (do_simple_request(rq, &nr_bytes)!= SUCCESS)
            status = BLK_STS_IOERR;

        printk(KERN_WARNING "sblkdev: request process% d bytes\n", nr_bytes);

#if 0 // proprietary module
        blk_mq_end_request (rq, status);
#else // can set real processed bytes count
        if (blk_update_request(rq, status, nr_bytes)) // GPL-only symbol
            BUG();
        __blk_mq_end_request(rq, status);
#endif
    }

    return BLK_STS_OK; // always return ok
}


/*
 * -----------------------------------------------------
 *     Helper functions for block device procedure
 * -----------------------------------------------------
 */
static int sblkdev_allocate_buffer(sblkdev_device_t* dev)
{
    // divide buffer size in bytes by sector shift to obtain size in sectors
    dev->capacity = _sblkdev_buffer_size >> SECTOR_SHIFT;
    // multiply size in sectors with sector shift to obtain size in bytes
    dev->data = kmalloc(dev->capacity << SECTOR_SHIFT, GFP_KERNEL); //
    if (dev->data == NULL) {
        printk(KERN_WARNING "sblkdev: vmalloc failure.\n");
        return -ENOMEM;
    }

    return SUCCESS;
}

static void sblkdev_free_buffer(sblkdev_device_t* dev)
{
    if (dev->data) {
        kfree(dev->data);

        dev->data = NULL;
        dev->capacity = 0;
    }
}


/*
 * -----------------------------------------------------
 *                 Block device procedure
 * -----------------------------------------------------
 */

/* 
 * Releases disk object and the queue, and then the buffers
 */
static void sblkdev_remove_device (void)
{
    sblkdev_device_t * dev = _sblkdev_device;

    printk (KERN_INFO "sblkdev: sblkdev_remove_device()\n");

    printk (KERN_INFO "sblkdev: %p\n", dev);
    if (dev) {
        if (dev-> disk)
            del_gendisk (dev-> disk);

        if (dev-> queue) {
            blk_cleanup_queue (dev-> queue);
            dev-> queue = NULL;
        }

        if (dev-> tag_set.tags)
            blk_mq_free_tag_set (& dev-> tag_set);

        if (dev-> disk) {
            put_disk (dev-> disk);
            dev-> disk = NULL;
        }

        sblkdev_free_buffer (dev);

        kfree (dev);
        _sblkdev_device = NULL;

        printk (KERN_WARNING "sblkdev: simple block device was removed\n");
    }
}

/*
 * Initialization of block device
 */
static int sblkdev_add_device (void)
{
    int ret = SUCCESS;

    /* allocate memory for device structure */
    /* kzalloc is faster and provides contiguous memory allocation */
    /* vmalloc does not need contiguous memory allocation, so it's good in case
    we need a lot of memory or our memory is fragmented, but it's slower */
    sblkdev_device_t * dev = kzalloc (sizeof (sblkdev_device_t), GFP_KERNEL);
    
    printk (KERN_INFO "sblkdev: %p\n", dev);

    printk (KERN_INFO "sblkdev: sblkdev_add_device()\n");

    if (dev == NULL) {
        printk (KERN_WARNING "sblkdev: unable to allocate% ld bytes\n", sizeof (sblkdev_device_t));
        return -ENOMEM;
    }
    _sblkdev_device = dev;
    printk (KERN_INFO "sblkdev: %p\n", dev);

    do {
        /* allocate memory buffer for data storage */
        ret = sblkdev_allocate_buffer (dev);
        if (ret)
            break;

/* initialize request queue - 1st alternative 
blk_mq_init_sq_queue() */
#if 0 // simply variant with helper function blk_mq_init_sq_queue. It`s available from kernel 4.20 (vanilla).
        {// configure tag_set
            struct request_queue * queue;

            dev-> tag_set.cmd_size = sizeof (sblkdev_cmd_t);
            dev-> tag_set.driver_data = dev;

            queue = blk_mq_init_sq_queue(&dev->tag_set, &_mq_ops, 128, BLK_MQ_F_SHOULD_MERGE | BLK_MQ_F_SG_MERGE);
            if (IS_ERR (queue)) {
                ret = PTR_ERR (queue);
                printk (KERN_WARNING "sblkdev: unable to allocate and initialize tag set  n");
                break;
            }
            dev-> queue = queue;
        }
/* initialize request queue - 2nd alternative 
blk_mq_alloc_tag_set() + blk_mq_init_queue(), which is equivalent to blk_mq_init_sq_queue() 
since it's just a wrapper function */
#else // more flexible variant
        {// configure tag_set
            dev->tag_set.ops = & _mq_ops;
            dev->tag_set.nr_hw_queues = 1;
            dev->tag_set.queue_depth = 128;
            dev->tag_set.numa_node = NUMA_NO_NODE;
            dev->tag_set.cmd_size = sizeof (sblkdev_cmd_t);
            dev->tag_set.flags = BLK_MQ_F_SHOULD_MERGE/* | BLK_MQ_F_SG_MERGE*/;
            dev->tag_set.driver_data = dev;

            ret = blk_mq_alloc_tag_set (& dev-> tag_set);
            if (ret) {
                printk (KERN_WARNING "sblkdev: unable to allocate tag set\n");
                break;
            }
        }

        {// configure queue
            struct request_queue * queue = blk_mq_init_queue (&dev->tag_set);
            if (IS_ERR (queue)) {
                ret = PTR_ERR (queue);
                printk (KERN_WARNING "sblkdev: Failed to allocate queue\n");
                break;
            }
            dev->queue = queue;
        }
#endif
        /*  */
        dev->queue->queuedata = dev;

        {// configure disk
            struct gendisk * disk = alloc_disk (1); // only one partition
            if (disk == NULL) {
                printk (KERN_WARNING "sblkdev: Failed to allocate disk\n");
                ret = -ENOMEM;
                break;
            }

            disk->flags |= GENHD_FL_NO_PART_SCAN; // only one partition
            // disk-> flags | = GENHD_FL_EXT_DEVT;
            disk->flags |= GENHD_FL_REMOVABLE;

            disk->major = _sblkdev_major;
            disk->first_minor = 0;
            disk->fops = & _fops;
            disk->private_data = dev;
            disk->queue = dev-> queue;
            // creates entry in /dev/blkdev0
            sprintf (disk->disk_name, "blkdev%d", 0);
            set_capacity (disk, dev->capacity);

            dev->disk = disk;
            add_disk (disk);
        }

        printk (KERN_WARNING "sblkdev: simple block device was created\n");
    } while (false);

    if (ret) {
        sblkdev_remove_device ();
        printk (KERN_WARNING "sblkdev: Failed add block device\n");
    }

    return ret;
}

static int __init sblkdev_init (void)
{
    int ret = SUCCESS;

    _sblkdev_major = register_blkdev(_sblkdev_major, _sblkdev_name);
    
    printk (KERN_INFO "sblkdev: sblkdev_init()\n");
    
    if (_sblkdev_major <= 0){
        printk(KERN_WARNING "sblkdev: unable to get major number\n");
        return -EBUSY;
    }

    ret = sblkdev_add_device();
    if (ret)
        unregister_blkdev(_sblkdev_major, _sblkdev_name);
        
    return ret;
}

static void __exit sblkdev_exit(void)
{
    printk (KERN_INFO "sblkdev: sblkdev_exit()\n");

    sblkdev_remove_device();

    if (_sblkdev_major > 0)
        unregister_blkdev(_sblkdev_major, _sblkdev_name);
}

MODULE_LICENSE("GPL");
module_init (sblkdev_init);
module_exit (sblkdev_exit);