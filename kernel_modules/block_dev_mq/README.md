# Writing a block device in kernel 5.0 with multi-queue block layer (blk-mq)

## Documentation
* https://prog.world/linux-kernel-5-0-we-write-simple-block-device-under-blk-mq/
* https://olegkutkov.me/2020/02/10/linux-block-device-driver/
* https://hyunyoung2.github.io/2016/09/14/Multi_Queue/


## multi-queue block layer (blk-mq)
* include/linux/blk-mq.h

* better for SSD since they can take advantage of parallel access to different chunks of data and fast random data access

### API

#### gendisk
```
struct gendisk {
    /* major, first_minor and minors are input parameters only,
     * don't use directly.  Use disk_devt() and disk_max_parts().
     */
    int major;          /* major number of driver */
    int first_minor;
    int minors;                     /* maximum number of minors, =1 for
                                         * disks that can't be partitioned. */

    char disk_name[DISK_NAME_LEN];  /* name of major driver */
    char *(*devnode)(struct gendisk *gd, umode_t *mode);

    unsigned short events;      /* supported events */
    unsigned short event_flags; /* flags related to event processing */

    /* Array of pointers to partitions indexed by partno.
     * Protected with matching bdev lock but stat and other
     * non-critical accesses use RCU.  Always access through
     * helpers.
     */
    struct disk_part_tbl __rcu *part_tbl;
    struct hd_struct part0;

    const struct block_device_operations *fops;
    struct request_queue *queue;
    void *private_data;

    int flags;
    struct rw_semaphore lookup_sem;
    struct kobject *slave_dir;

    struct timer_rand_state *random;
    atomic_t sync_io;       /* RAID */
    struct disk_events *ev;
#ifdef  CONFIG_BLK_DEV_INTEGRITY
    struct kobject integrity_kobj;
#endif /* CONFIG_BLK_DEV_INTEGRITY */
    int node_id;
    struct badblocks *bb;
    struct lockdep_map lockdep_map;
};
```

```
struct gendisk *alloc_disk(int minors) /* allocate new gendisk structure, minors is the maximum number of minor numbers (partitions) that this disk can have */

void add_disk(struct gendisk *disk); /* add new disk to the system. Please note that gendisk structure must be initialized before adding new disk */

void set_capacity(struct gendisk *disk, sector_t size); /* specify capacity (in sectors) of the new disk */

void del_gendisk(struct gendisk *gp); /* delete disk from the system */

void put_disk(struct gendisk *disk); /* release memory allocated in alloc_disk(int minors) */
```

* block device initialization kernel 4:
```
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/fs.h>

int blockdev_open(struct block_device *dev, fmode_t mode)
{
    printk("Device %s opened"\n, dev->bd_disk->disk_name);
    return 0;
}

void blockdev_release(struct gendisk *gdisk, fmode_t mode)
{
    printk("Device %s closed"\n, dev->bd_disk->disk_name);
}

int blockdev_ioctl (struct block_device *dev, fmode_t mode, unsigned cmd, unsigned long arg)
{
    return -ENOTTY; /* ioctl not supported */
}

static struct block_device_operations blockdev_ops = {
    .owner = THIS_MODULE,
    .open = blockdev_open,
    .release = blockdev_release,
    .ioctl = blockdev_ioctl
};

static void block_request(struct request_queue *q)
{
    int direction;
    int err = -EIO;
    u8 *data;
    struct request *req = blk_fetch_request(q); /* get one top request from the queue */

    while (req) {
        if (__blk_end_request_cur(req, err)) {  /* check for the end */
            break;
        }

        /* Data processing */

        direction = rq_data_dir(req);

        if (direction == WRITE) {
            printk("Writing data to the block device\n");
        } else {
            printk("Reading data from the block devicen\n");
        }

        data = bio_data(req->bio); /* Data buffer to perform I/O operations */

        /* */

        req = blk_fetch_request(q); /* get next request from the queue */
    }
}

void block_dev_init()
{
    spinlock_t *lk;  /* Main lock for the device */
    struct gendisk *gdisk;
    int major_number = register_blkdev(0, "testblk");

    gdisk = alloc_disk(1);

    if (!gdisk) {
        return;
    }

    spin_lock_init(&lk);

    snprintf(gdisk->disk_name, 8, "blockdev");  /* Block device file name: "/dev/blockdev" */

    gdisk->flags = GENHD_FL_NO_PART_SCAN;  /* Kernel won't scan for partitions on the new disk */
    gdisk->major = major_number;
    gdisk->fops = &blockdev_ops;  /* Block device file operations, see below */
    gdisk->first_minor = 0;

    gdisk->queue = blk_init_queue(req_fun, &lk);  /* Init I/O queue, see below */

    set_capacity(block_device->gdisk, 1024 * 512);  /* Set some random capacity, 1024 sectors (with size of 512 bytes) */

    add_disk(gdisk);
}
``` 

* block device initialization kernel >= 5.0 (blk_mq framework):
    * blk_mq_init_sq_queue() instead of blk_init_queue():
        * this function is a simple wrapper around blk_alloc_queue_node() and blk_mq_init_allocated_queue() and implements a single hardware queue. If you want to support more, you need to write your own wrapper function
    ```
    struct request_queue *blk_mq_init_sq_queue(struct               blk_mq_tag_set *set,
                        const struct blk_mq_ops *ops,
                        unsigned int queue_depth,
                        unsigned int set_flags);
    ```

    * struct blk_mq_tag_set stores all params like driver private data, commands size, max queue depth, ... (it used to be struct blk_mq_reg):
    ```
    struct blk_mq_tag_set {
        /*
        * map[] holds ctx -> hctx mappings, one map exists for each type
        * that the driver wishes to support. There are no restrictions
        * on maps being of the same size, and it's perfectly legal to
        * share maps between types.
        */
        struct blk_mq_queue_map map[HCTX_MAX_TYPES];
        unsigned int        nr_maps;    /* nr entries in map[] */
        const struct blk_mq_ops *ops;
        unsigned int        nr_hw_queues;   /* nr hw queues across maps */
        unsigned int        queue_depth;    /* max hw supported */
        unsigned int        reserved_tags;
        unsigned int        cmd_size;   /* per-request extra data */
        int         numa_node;
        unsigned int        timeout;
        unsigned int        flags;      /* BLK_MQ_F_* */
        void            *driver_data;

        struct blk_mq_tags  **tags;

        struct mutex        tag_list_lock;
        struct list_head    tag_list;
    };
    ```

    * struct blk_mq_ops is a set of function pointers

    * queue_rq() must be implemented to serve I/O requests. It's equivalent to the previous request_fn_proc()

```
#include <linux/blk-mq.h>

int do_request(struct request *rq, unsigned int *nr_bytes)
{
    struct bio_vec bvec;
    struct req_iterator iter;
    loff_t pos = blk_rq_pos(rq) << SECTOR_SHIFT; /* Required position for the Read or Write */
    void* data;
    unsigned long data_len;

    /* Iterate over reuquests in the queue */    
    rq_for_each_segment(bvec, rq, iter)
    {
        data = page_address(bvec.bv_page) + bvec.bv_offset; /* Get I/O data */
        data_len = bvec.bv_len; /* Length of the data buffer */

        if (rq_data_dir(rq) == WRITE) {
            printk("Writing data to the blk-mq device\n");
        } else {
            printk("Reading data from the blk-mq device\n");
        }

        pos += b_len;
        *nr_bytes += data_len;  /* Increment amount of the processed bytes. */
    }

    return 0;
}

/* Simple example of the blk-mq I/O function */
blk_status_t queue_rq(struct blk_mq_hw_ctx *hctx, const struct blk_mq_queue_data* bd)
{   
    unsigned int nr_bytes = 0;
    blk_status_t status = BLK_STS_OK;
    struct request *rq = bd->rq;  /* Get one top request */

    /* 
        Start new request procedure.
        Please note that this function sets a timer for the data transaction. 
        This internal timer is used to measure request processing time and to detect problems with hardware. 
    */
    blk_mq_start_request(rq);

    /* Serve request */     
    if (do_request(rq, &nr_bytes) != 0) {
        status = BLK_STS_IOERR;
    }

    /* Notify blk-mq about processed bytes */
    if (blk_update_request(rq, status, nr_bytes)) {
        BUG();
    }

    /* End request procedure */     
    __blk_mq_end_request(rq, status);
     
    return status;
}

/* Set I/O function */
static struct blk_mq_ops mq_ops = {
    .queue_rq = queue_rq,
};

void block_dev_init()
{
    struct blk_mq_tag_set tag_set;

    /* */

    /* Create new queue with depth 128 and with on flag to enable queues merging. */
    gdisk->queue = blk_mq_init_sq_queue(&tag_set, &mq_ops, 128, BLK_MQ_F_SHOULD_MERGE);

    /* */
}
```

* notes for RAM drive example:
    * we can use a memory buffer to simulate real hardware
    * the size of the buffer must be equal to the capacity set in set_capacity()
    * we can use memcpy() because the kernel block device subsystem handles all nuances of copying to and from the user 


## dd tool
### syntax
* if: input source
* of: output source

* bs: block size, can be omitted, defaults to 512; corresponds to the number of bytes to copy at a single time
* count: amount of blocks to be read

* skip: skips x blocks
* seek: is the same as skip but the other way around, when we want to restore data instead of backing up

### examples
* copy an entire drive:
```
dd if=/dev/sda of=/dev/sdb
```

* restore an entire drive:
```
dd if=/dev/sdb of=/dev/sda
```

* backup MBR, with sector of length 512 bytes and stage 1:
```
$ sudo dd if=/dev/sda bs=512 count=1 of=mbr.img
```

* If we want to skip the first part of the MBR to backup the hidden data between the MBR and the first partition of the disk, which usually starts at sector 2048. The remaining 2047 that are not part of the MBR can be backed up with the following (this copies 2047 blocks of 512 bytes from /dev/sd starting on the second block):
```
$ sudo dd if=/dev/sda of=hidden-data-after-mbr count=2047 skip=1
```

* Overwrite disk with zeros:
```
$ sudo dd if=/dev/zero bs=1M of=/dev/sda
```

* Overwrite disk with random data:
```
$ sudo dd if=/dev/urandom bs=1M of=/dev/sda
```

#### Writing data to a block device
* write file hello.txt into 
dd if=hello.txt of=dev/blkdev0 


## fdisk tool
* List partitions:
```
sudo fdisk -l 
```

* List partitions on device /dev/sda:
```
sudo fdisk -l /dev/sda
```

* Enter command mode for device /dev/sda:
```
sudo fdisk /dev/sda
```

* List all commands available in command mode:
```
m
```

* View partition table:
```
p
```

* Deleting a partition (then insert id from the p command):
```
d
```

* Creating a partition:
    * l for logical partition
    * p for primary partition
        * no difference when it comes just to storing data
```
n
```

* Start fresh with a disk by destroying the partition table and creating a new empty DOS partition table:
```
o
```

* Quit without saving changes:
```
q
```

* Quit saving changes:
```
m
```

* Verify if there are errors:
```
v
```

* NOTE: creating partitions with dd requires reboot, which will unload the kernel module, so partitions need to be created another way



## RAM drive
* https://www.linuxjournal.com/article/2890

* Linux source code: 
    * RAM drive: drivers/block/ramdisk.c


## General Procedure
* At initialization :
1. Register the module as block device with register_blkdev
2. Allocate the device data buffer
3. Initialize spinlock
4. Initialize request queue
5. Configure request queue
6. Allocate gendisk structure
7. Fill gendisk structure
8. Create the disk with add_disk