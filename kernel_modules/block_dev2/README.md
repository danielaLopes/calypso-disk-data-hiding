# Documentation
* followed https://linux-kernel-labs.github.io/refs/heads/master/labs/block_device_drivers.html#lab-objectives
* Block device lecture: https://hyunyoung2.github.io/img/Image/SSD-Solid_State_Drives/2016-09-08-Block_Device/Lecture_4_-_Storage_Systems_in_the_Kernel.pdf
* https://hyunyoung2.github.io/2016/09/08/Block_Device/
* The journey of an I/O request through the block layer: https://users.suse.com/~sjayaraman/talks/IO_Journey_osidays.pdf
* VFS, block layer and device drivers: http://blog.vmsplice.net/2020/04/how-linux-vfs-block-layer-and-device.html
* http://static.lwn.net/images/pdf/LDD3/ch16.pdf

## Source code
* sbull: https://github.com/starpos/scull/tree/master/sbull
* sbull updated for kernel 5.0: https://github.com/martinezjavier/ldd3/tree/master/sbull (this repository has updated code for Linux Device Drivers 3rd Edition)


# Overview
* It is more complicated than working with character devices because character devices have a single current position, while block devices must be able to move to any position in the device to provide random access to data. 
    * For this, the Linux krenel provides the subsystem called the block I/O or block layer subsystem

* the smallest unit is the sector; the size of the block must be a multiple of the size of the sector

* communications between de device and the driver will be performed in sectors of 512 bytes. All requests by the kernel will be multipe of this size, so a conversion should be done by calling set_capacity()


# API
## block_device
* linux/fs.h

* register: performs dynamic allocation of a major in case o major = 0 or statically allocates the speficied major and creates an entry in /proc/devices. to be called in module init function
```
/*
 * Major 0 enables the OS to dynamically choose a major
 * Allocates the major number as an entry in /proc/dev
 */
register_blkdev(unsigned int major, const char * name);
```

* unregister: 
```
unregister_blkdev(unsigned int major, const char * name);
```


### structure gendisk
* in linux/genhd.h
* individual disk device
```
struct gendisk {
    /* describing the identifiers used by the disk; a disk must have at least one minor; if the disk allows the partitioning operation, a minor must be allocated for each possible partition */    
    int major;
    int first_minor;
    int minor; 

    struct hd_struct ** part; /*  */

    char disk_name[32]; /* represents the disk name as it appears in /proc/partitions and in sysfs (/sys/block) */

    struct block_device_operations * fops /* representing operations associated with the disk; struct structure block_device_operations */

    struct request_queue queue /* which represents the queue of requests */

    sector_t capacity /* which is disk capacity in 512 byte sectors; it is initialized using the set_capacity() function */

    private_data /* which is a pointer to private data */
}
```

* Create block device (disk). register_blkdev() does not provide a device to the system.
    * allocate a disk: (in init module)
    ```
    /*
     * minors is the number of minor numbers this disk uses
     */ 
    alloc_disk(int minors);
    ```

    * adding disk to the system: (in init module)
    ```
    /*
     * must be called after alloc_disk() to make the disk available to the system
     * should only be called after the driver is completely initialized
     */
    add_disk(struct gendisk *gd);
    ```

    * deallocate a disk: (in exit module)
    ```
    del_gendisk(struct gendisk *gd);
    ```

### gendisk vs hd_struct
* represent disk partition

### structure block_device_operations
* linux/fs.h

* equivalent to struct file_operations for character devices

```
struct block_device_operations {
    int (*open) (struct block_device *, fmode_t); /* increments usage counter */

    int (*release) (struct gendisk *, fmode_t); /* decrements usage counter */

    int (*locked_ioctl) (struct block_device *, fmode_t, unsigned,
                         unsigned long);

    int (*ioctl) (struct block_device *, fmode_t, unsigned, unsigned long);

    int (*compat_ioctl) (struct block_device *, fmode_t, unsigned,
                         unsigned long);

    int (*direct_access) (struct block_device *, sector_t,
                          void **, unsigned long *); /* enables reference to data on storage system */

    int (*media_changed) (struct gendisk *); /* supports removable media */

    int (*revalidate_disk) (struct gendisk *);

    /* geometry has to do with the hardware, such as tracks and cylinders. The fdisk tool queries this because it edits partition tables depending on the cylinder information */
    int (*getgeo)(struct block_device *, struct hd_geometry *); /* returns geometry of the device */

    struct module *owner;
}
```

* open() and release() are called directly from user space by utilities that perform partitioning, file system creation and file system verification

* in mount(), the open() function is called directly from the kernel space, the file descriptor is being stored by the kernel

* a driver for a block device cannot diferentiate between open() calls performed from user space and kernel space


### Request queues
* linux/blkdev.h

* there are no read() or write() operations because these are performed by the request() function associated with the request queue of the disk

* drivers for block devices use queues to store the block I/O requests that will be processed.

* It's represented by the structure request_queue

* It's a double linked list of requests and their associated control information, which are added to the queue by higher-level kernel code

* As long as the request queue is not empty, the queue's associated driver will have to retrieve the first request from the queue and pass it to the associated block device

* Each item in the request queue is represented by the structure request.

* Request queues implement an interface that allows the use of multiple I/O schedulers. The scheduler must:
    * sort the requests and present them to the driver in order to maximize performance
    * The scheduler also deals with the combination of adjacent requests (which refer to adjacent sectors of the disk)

```
struct request_queue {
    struct list_head queue_head; /* pointer to a list with requests */

    // ...

    elevator_t *elevator; /* pointer to elevator queue (schedular and functions) */

    // ...

    request_fn_proc *request_fn; /* pointer to request function of the device */

    // ...
}
```

* Create request queues:
```
blk_init_queue()
```

* Delete request queues: 
```
blk_cleanup_queue()
```

* functions used to process the requests from the request queue: (each access to these functions required holding the spinlock of the queue)
```
/* retrieves a reference to the first request from the queue; the respective request must be started using blk_start_request() */
blk_peek_request() 

/* extracts the request from the queue and starts it for processing; in general, the function receives as a reference a pointer to a request returned by blk_peek_request() */
blk_start_request()

/* retrieves the first request from the queue (using blk_peek_request()) and starts it (using blk_start_request()) */
blk_fetch_request()

/* to re-enter queue */
blk_requeue_request()
```


#### Requests
* Parameters:
```
cmd_flags   /* a series of flags including direction (reading or writing); to find out the direction, the macrodefinition rq_data_dir is used, which returns 0 for a read request and 1 for a write request on the device */

__sector    /* the first sector of the transfer request; if the device sector has a different size, the appropriate conversion should be done. To access this field, use the blk_rq_pos macro */

__data_len  /* the total number of bytes to be transferred; to access this field the blk_rq_bytes macro is used
generally, data from the current struct bio will be transferred; the data size is obtained using the blk_rq_cur_bytes macro */

bio         /* a dynamic list of struct bio structures that is a set of buffers associated to the request; this field is accessed by macrodefinition rq_for_each_segment if there are multiple buffers, or by bio_data macrodefinition in case there is only one associated buffer */
```

* Create a request:

* Finish a request:

* Process a request:


##### structure bio
* linux/bio.h
* https://lwn.net/Articles/26404/

* I/O block request. Each request is a bio
* The sectors to be transferred for a request can be scattered into the main memory but they always correspond to a set of consecutive sectors on the device.
* The request is represented as a series of segments, each corresponding to a buffer in memory.
* The kernel can combine requests that refer to adjacent sectors but will not combine write requests with read requests into a single struct request.

* A struct request is implemented as a linked list of struct bio structures together with information that allows the driver to retain its current position while processing the request

```
struct request {
	struct request_queue *q;
	struct blk_mq_ctx *mq_ctx; /* state for a software queue */
	struct blk_mq_hw_ctx *mq_hctx;

	unsigned int cmd_flags;		/* op and common flags */
	req_flags_t rq_flags;

	int tag;
	int internal_tag;

	/* the following two fields are internal, NEVER access directly */
	unsigned int __data_len;	/* total data len */
	sector_t __sector;		/* sector cursor */

	struct bio *bio;
	struct bio *biotail;

	struct list_head queuelist;
    \\ ...
```

```
struct bio {
    //...

    struct gendisk          *bi_disk;

    unsigned int            bi_opf;         /* bottom bits req flags, top bits REQ_OP. Use accessors. Encodes the request type. To determine it, use bio_data_dir() */

    //...

    struct bio_vec          *bi_io_vec;     /* the actual vec list. Consists on the individual pages in physical memory to be transferred, the offset within the page and the size of the buffer. To iterate through bio, we need to iterate through this vector and transfer the data from every physical page */

    //...


    struct bvec_iter        bi_iter;        /* Used to simplify iteration. It maintains information about how many buffers and sectors were consume during the iteration */
    /...

    void                    *bi_private;

    //...
};
```

```
struct bio_vec {
    struct page *bv_page;
    unsigned int bv_len;
    unsigned int bv_offset;
};
```

1. Create:
    * bio_alloc(): allocates space for a new structure, which must be initialized
    * bio_clone(): makes a copy of an existing struct bio. The new structure is initialized with the values of the clones structure fields; the buffers are shared with struct bio that has been cloned, so that access to the buffers has to be done carefully to avoid access to the same memory area from the two clones

2. Initialize:
    * bio_add_page(struct bio *bio, struct page *page, unsigned int len,unsigned int offset): The content of a struct bio is a buffer described by: a physical page, the offset in the page and the size of the buffer. A physical page can be assigned using the alloc_page()
```
struct bio *bio = bio_alloc(GFP_NOIO, 1);
//...
// We need to fill its important fields before submitting it
bio->bi_disk = bdev->bd_disk;
bio->bi_iter.bi_sector = sector; /* specify startup sector */
bio->bi_opf = REQ_OP_READ; /* type of operation read or write */
bio_add_page(bio, page, size, offset);
//...
```

3. Submit: requests are not processed right away. This adds a struct bio to a request from the request queue to be processed.
    * submit_bio(struct bio * bio): receives as argument an initialized struct bio that will be added to a request from the request queue of an I/O device. Does not wait for the request to be finished. But we can still check if the request was completed by consulting the bi_end_io field of the structure, which specifies the function called when the processing of the structure is finished. We can use the bi_private field to pass information to that function
    * submit_bio_wait(struct bio * bio): waits for the processing of the request to be finished

4. Free: bio_put(struct bio * bio): the last put of a bio frees it


# Important Concepts
## elevator algorithm
* improve performance of I/O operations by grouping continguous sectors and sorting requests in the request queue

* set of methods: elevator_ops

* different schedulers: noop, cfq, deadline. Can be switched during runtime

# Notes from reading the book
* the request implementation should not need information apart from the information in the request and bio structures; accessing info in the process that issued the request or something similar is not correct
* the bio structure has a field called bi_size that corresponds to the size of the data to be transferred in bytes, or if we prefer, we can use the function bio_sectors(bio) that returns the size in sectors

## bio structure and request()
* The block layer also maintains a set of pointers within the bio structure to keep track of the current state of request processing. Several macros exist to provide access to that state:
```
struct page *bio_page(struct bio *bio);
Returns a pointer to the page structure representing the page to be transferred next.
int bio_offset(struct bio *bio);
Returns the offset within the page for the data to be transferred.
int bio_cur_sectors(struct bio *bio);
Returns the number of sectors to be transferred out of the current page.
char *bio_data(struct bio *bio);
Returns a kernel logical address pointing to the data to be transferred. Note that this address is available only if the page in question is not located in high mem- ory; calling it in other situations is a bug. By default, the block subsystem does not pass high-memory buffers to your driver, but if you have changed that set- ting with blk_queue_bounce_limit, you probably should not be using bio_data.
void bio_kunmap_irq(char *buffer, unsigned long *flags);
bio_kmap_irq returns a kernel virtual address for any buffer, regardless of whether it resides in high or low memory. An atomic kmap is used, so your driver cannot sleep while this mapping is active. Use bio_kunmap_irq to unmap the buffer. Note that the flags argument is passed by pointer here. Note also that since an atomic kmap is used, you cannot map more than one segment at a time.
```
* All of the functions just described access the “current” buffer—the first buffer that, as far as the kernel knows, has not been transferred. Drivers often want to work through several buffers in the bio before signaling completion on any of them (with end_that_request_first, to be described shortly), so these functions are often not use- ful. Several other macros exist for working with the internals of the bio structure (see <linux/bio.h> for details).