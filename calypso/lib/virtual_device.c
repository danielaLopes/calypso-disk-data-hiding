/*
 *
 */
#include <linux/bitmap.h>
#include <linux/genhd.h>

#include "global.h"
#include "debug.h"
#include "virtual_device.h"


int calypso_dev_init_bitmaps(struct calypso_blk_device *calypso_dev)
{
    if (calypso_dev)
    {
        calypso_dev->high_entropy_blocks_bitmap = bitmap_zalloc(calypso_dev->physical_nr_blocks, GFP_KERNEL);
        if (!calypso_dev->high_entropy_blocks_bitmap)
        {
            debug(KERN_ERR, __func__, "Could not allocate memory for high entropy blocks bitmap\n");
            return -ENOMEM;
        }

        debug_args(KERN_DEBUG, __func__, "virtual dev has %ld blocks\n", calypso_dev->virtual_nr_blocks);
        debug_args(KERN_DEBUG, __func__, "physical dev has %ld blocks\n", calypso_dev->physical_nr_blocks);
        calypso_dev->physical_blocks_bitmap = bitmap_zalloc(calypso_dev->physical_nr_blocks, GFP_KERNEL);
        if (!calypso_dev->physical_blocks_bitmap)
        {
            debug(KERN_ERR, __func__, "Could not allocate memory for physical bitmap\n");
            return -ENOMEM;
        }
    }
    debug(KERN_DEBUG, __func__, "Allocated bitmaps successfully\n");
    return 0;
}

void calypso_dev_cleanup_bitmaps(struct calypso_blk_device *calypso_dev)
{
    if (calypso_dev)
    {
        if (calypso_dev->high_entropy_blocks_bitmap)
            bitmap_free(calypso_dev->high_entropy_blocks_bitmap);

        if (calypso_dev->physical_blocks_bitmap)
            bitmap_free(calypso_dev->physical_blocks_bitmap);
    }
}

static void _calypso_set_mapping_to_value(unsigned long *mappings, unsigned long value, unsigned long size)
{
    unsigned long i;
    for (i = 0; i < size; i++)
        mappings[i] = value;
}

int calypso_dev_init_mappings(struct calypso_blk_device *calypso_dev)
{
    if (calypso_dev)
    {
        /* since this is a big allocations, we cannot rely on contiguous memory */
        calypso_dev->metadata_to_physical_block_mapping = vmalloc(calypso_dev->metadata_nr_blocks * sizeof(unsigned long));
        if (!calypso_dev->metadata_to_physical_block_mapping)
        {
            debug(KERN_ERR, __func__, "Could not allocate memory for metadata to physical mappings vector\n");
            return -ENOMEM;
        }
        _calypso_set_mapping_to_value(calypso_dev->metadata_to_physical_block_mapping, -1, calypso_dev->metadata_nr_blocks);

        calypso_dev->virtual_to_physical_block_mapping = vmalloc(calypso_dev->virtual_nr_blocks * sizeof(unsigned long));
        if (!calypso_dev->virtual_to_physical_block_mapping)
        {
            debug(KERN_ERR, __func__, "Could not allocate memory for virtual to physical mappings vector\n");
            return -ENOMEM;
        }
        _calypso_set_mapping_to_value(calypso_dev->virtual_to_physical_block_mapping, -1, calypso_dev->virtual_nr_blocks);

        calypso_dev->physical_to_virtual_block_mapping = vmalloc(calypso_dev->physical_nr_blocks * sizeof(unsigned long));
        if (!calypso_dev->physical_to_virtual_block_mapping)
        {
            debug(KERN_ERR, __func__, "Could not allocate memory for physical to virtual mappings vector\n");
            return -ENOMEM;
        }
        _calypso_set_mapping_to_value(calypso_dev->physical_to_virtual_block_mapping, -1, calypso_dev->physical_nr_blocks);
    }
    return 0;
}

void calypso_dev_cleanup_mappings(struct calypso_blk_device *calypso_dev)
{
    if (calypso_dev)
    {
        if (calypso_dev->metadata_to_physical_block_mapping)
            vfree(calypso_dev->metadata_to_physical_block_mapping);

        if (calypso_dev->virtual_to_physical_block_mapping)
            vfree(calypso_dev->virtual_to_physical_block_mapping);

        if (calypso_dev->physical_to_virtual_block_mapping)
            vfree(calypso_dev->physical_to_virtual_block_mapping);
    }
}

void calypso_set_physical_partition_sector_ranges(struct calypso_blk_device *calypso_dev)
{
    calypso_dev->first_physical_sector = get_start_sect(calypso_dev->physical_dev);
    /* nr_sects is not atomic and needs to be protected by a partition or gendisk mutex */
    calypso_dev->last_physical_sector = calypso_dev->first_physical_sector + part_nr_sects_read(calypso_dev->physical_dev->bd_part) - 1;
    debug_args(KERN_DEBUG, __func__, "First physical sector %llu\n", calypso_dev->first_physical_sector);
    debug_args(KERN_DEBUG, __func__, "Last physical sector %llu\n", calypso_dev->last_physical_sector);
}

unsigned long calypso_get_block_nr_from_sector(sector_t cur_sector, sector_t first_sector)
{
    return (cur_sector - first_sector) / 8;
}

sector_t calypso_get_sector_nr_from_block(unsigned long block, unsigned long offset)
{
    // TODO: see this better
    /* Here we don't add the first_sector's offset because the request is being issued to a specific partition?
    And this function is only used when remapping */
    // return (block * 8 + first_sector);
    /* offset is in bytes, so we need to pass it to sectors */
    sector_t physical_sector_nr = block * 8 + offset / DISK_SECTOR_SIZE;
    //sector_t physical_sector_nr = block * 8;
    // debug_args(KERN_INFO, __func__, "physical sector number to write %llu\n", physical_sector_nr);
    return physical_sector_nr;
}

unsigned long calypso_get_free_blocks_group(unsigned long *bitmap, unsigned long last_block, unsigned int nblocks)
{   
    return bitmap_find_next_zero_area(bitmap, last_block, 0, nblocks, 0);
}

/*
 * free_block parameter needs to be initialized with the value of the block 
 * we want to start looking for. So if we want to start looking from the
 * first block of the disk, we need to set free_block to 0UL before 
 * passing it to this function
 * 
 * Returns 0 for success and -1 for error
 */
int calypso_get_next_free_block(unsigned long *bitmap, unsigned long last_block, unsigned long *free_block)
{   
    unsigned long initial_value = *free_block;
    // *free_block = find_next_zero_bit(bitmap, last_block, *free_block);
    *free_block = find_next_bit(bitmap, last_block, *free_block);

    /* 
     * block 0 is never going to be free on a partition formatted with ext4,
     * so we do not have to worry about the next free block being 0 
     */
    if (*free_block == initial_value) {
        debug(KERN_ERR, __func__, "Could not find next free block\n");
        return -1;
    }
    return 0;
}

void calypso_set_bit(unsigned long *bitmap, unsigned long start)
{
    /* only updates one bit in position start */
    bitmap_set(bitmap, start, 1);
}

void calypso_clear_bit(unsigned long *bitmap, unsigned long start)
{
    /* only updates one bit in position start */
    bitmap_clear(bitmap, start, 1);
}

void calypso_update_bitmaps(unsigned long *physical_blocks_bitmap, unsigned long *high_entropy_blocks_bitmap, unsigned long start)
{
    bitmap_set(physical_blocks_bitmap, start, 1);
    bitmap_clear(high_entropy_blocks_bitmap, start, 1);
}

int calypso_dev_init(struct calypso_blk_device **calypso_dev)
{
    (*calypso_dev) = kzalloc(sizeof(struct calypso_blk_device), GFP_KERNEL);
	if (!(*calypso_dev)) {
        debug(KERN_WARNING, __func__, "Unable to allocate bytes for Calypso device\n");
        return -ENOMEM;
    }
    debug(KERN_INFO, __func__, "Allocated memory for device struct\n");

    return 0;
}

void calypso_print_queue_data(struct request_queue *queue) 
{
    struct queue_limits *lim = &(queue->limits);

    debug(KERN_INFO, __func__, "Request queue data:\n");
    
    debug_args(KERN_INFO, __func__, "max_sectors: %d\n", lim->max_sectors);
    debug_args(KERN_INFO, __func__, "max_dev_sectors: %d\n", lim->max_dev_sectors);
    debug_args(KERN_INFO, __func__, "max_hw_sectors: %d\n", lim->max_hw_sectors);
    debug_args(KERN_INFO, __func__, "logical_block_size: %d\n", lim->logical_block_size);
    debug_args(KERN_INFO, __func__, "physical_block_size: %d\n", lim->physical_block_size);
    debug_args(KERN_INFO, __func__, "max_segments: %d\n", lim->max_segments);
    // debug_args(KERN_INFO, __func__, "max_hw_segments: %d\n", queue_max_hw_segments(queue));
    // debug_args(KERN_INFO, __func__, "hardsect_size: %d\n", queue_hardsect_size(queue));
    debug_args(KERN_INFO, __func__, "max_segment_size: %d\n", lim->max_segment_size);
    // debug_args(KERN_INFO, __func__, "max_sectors: %d\n", queue_max_sectors(queue));
    // debug_args(KERN_INFO, __func__, "max_hw_sectors: %d\n", queue_max_hw_sectors(queue));
    // debug_args(KERN_INFO, __func__, "max_phys_segments: %d\n", queue_max_phys_segments(queue));
    // debug_args(KERN_INFO, __func__, "max_hw_segments: %d\n", queue_max_hw_segments(queue));
    // debug_args(KERN_INFO, __func__, "hardsect_size: %d\n", queue_hardsect_size(queue));
    // debug_args(KERN_INFO, __func__, "max_segment_size: %d", queue_max_segment_size(queue));

    //blk_queue_max_segment_size(queue, 512);
}

int calypso_dev_setup(struct calypso_blk_device *calypso_dev, 
                    struct block_device_operations *calypso_fops, 
                    struct blk_mq_ops *calypso_fops_req, 
                    const char *dev_name,
                    int *calypso_major)
{
    char part_name[80] = "";

    (*calypso_major) = register_blkdev((*calypso_major), dev_name);
    if ((*calypso_major) <= 0)
    {
        debug(KERN_ERR, __func__, "Unable to get Major Number\n");
        return -EBUSY;
    }
    debug_args(KERN_INFO, __func__, "Registered block device with major %d\n", (*calypso_major)); 

    // TODO: change this
    //calypso_dev->next_sector_to_write = START_SECTOR;

    /* Get a request queue (here queue is created) */
    spin_lock_init(&(calypso_dev->lock));
    calypso_dev->queue = blk_mq_init_sq_queue(&(calypso_dev->tag_set), calypso_fops_req, 128, BLK_MQ_F_SHOULD_MERGE);
    if (IS_ERR(calypso_dev->queue)) {
        debug(KERN_ERR, __func__, "blk_mq_init_sq_queue failure\n");
        calypso_memory_cleanup(calypso_dev);
        unregister_blkdev((*calypso_major), dev_name);
        return -ENOMEM;
    }
    /* sets logical block size for the queue */
    // TODO: understand what this function does if the block size continues to be 4096
    /* this size just refers to the LBA, not the actual file system block size */
    blk_queue_logical_block_size(calypso_dev->queue, KERNEL_SECTOR_SIZE);
    // TODO:
    debug_args(KERN_INFO, __func__, "calypso_dev->queue->limits.physical_block_size: %d\n", calypso_dev->queue->limits.physical_block_size);
    // blk_queue_physical_block_size(calypso_dev->queue, 512);
	calypso_dev->queue->queuedata = calypso_dev;

    //print_queue_data(calypso_dev->queue);

    debug(KERN_INFO, __func__, "Set up request queue\n");
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
    calypso_dev->disk = alloc_disk(CALYPSO_MINOR_CNT);
    if (!calypso_dev->disk)
    {
        debug(KERN_ERR, __func__, "alloc_disk failure\n");
        blk_cleanup_queue(calypso_dev->queue);
        calypso_memory_cleanup(calypso_dev);
        unregister_blkdev((*calypso_major), dev_name);
        return -ENOMEM;
    }
    debug(KERN_INFO, __func__, "Allocated disk\n");

    /* can only have one partition 
    otherwise we need to create a partition table */
    calypso_dev->disk->flags |= GENHD_FL_NO_PART_SCAN;

    /* Setting the major number */
    calypso_dev->disk->major = (*calypso_major);
    /* Setting the first mior number */
    calypso_dev->disk->first_minor = CALYPSO_FIRST_MINOR;
    /* Initializing the device operations */
    calypso_dev->disk->fops = calypso_fops;
    /* Driver-specific own internal data */
    calypso_dev->disk->private_data = calypso_dev;
    calypso_dev->disk->queue = calypso_dev->queue;
    /*
     * You do not want partition information to show up in 
     * cat /proc/partitions set this flags
     */
    strcat(part_name, dev_name);
    strcat(part_name, "0");
    sprintf(calypso_dev->disk->disk_name, part_name);

    /* Setting the capacity of the device in its gendisk structure */
    set_capacity(calypso_dev->disk, calypso_dev->capacity);
    debug(KERN_INFO, __func__, "Set disk capacity\n");
 
    /* Adding the disk to the system */
    add_disk(calypso_dev->disk);
    /* Now the disk is "live" */
    debug(KERN_INFO, __func__, "Calypso device initialised\n");

    return 0;
}

// #if LINUX_VERSION_CODE <= KERNEL_VERSION(5,8,0)
// int calypso_dev_no_queue_setup(struct calypso_blk_device *calypso_dev, 
//                     struct block_device_operations *calypso_fops,
//                     const char *dev_name,
//                     int *calypso_major,
//                     blk_qc_t calypso_make_request(struct bio *bio))
// #else
int calypso_dev_no_queue_setup(struct calypso_blk_device *calypso_dev, 
                    struct block_device_operations *calypso_fops,
                    const char *dev_name,
                    int *calypso_major,
                    blk_qc_t (*calypso_make_request)(struct request_queue *q, struct bio *bio))
//#endif
{
    char part_name[80] = "";

    (*calypso_major) = register_blkdev((*calypso_major), dev_name);
    if ((*calypso_major) <= 0)
    {
        debug(KERN_ERR, __func__, "Unable to get Major Number\n");
        return -EBUSY;
    }
    debug_args(KERN_INFO, __func__, "Registered block device with major %d\n", (*calypso_major)); 

    /* Setup queue for handling requests */
    spin_lock_init(&(calypso_dev->lock));
    /* by using blk_alloc_queue() instead of blk_mq_init_sq_queue(), it does not setup a new queue for requests */

    calypso_dev->queue = blk_alloc_queue(GFP_KERNEL);
    //calypso_dev->queue = blk_alloc_queue(calypso_make_request, GFP_KERNEL);

    if (IS_ERR(calypso_dev->queue)) {
        debug(KERN_ERR, __func__, "blk_alloc_queue failure\n");
        calypso_memory_cleanup(calypso_dev);
        unregister_blkdev((*calypso_major), dev_name);
        return -ENOMEM;
    }

    blk_queue_make_request(calypso_dev->queue, calypso_make_request);

    /* sets logical block size for the queue */
    blk_queue_logical_block_size(calypso_dev->queue, KERNEL_SECTOR_SIZE);
    debug_args(KERN_INFO, __func__, "calypso_dev->queue->limits.physical_block_size: %d\n", calypso_dev->queue->limits.physical_block_size);
    // blk_queue_physical_block_size(calypso_dev->queue, 512);
	calypso_dev->queue->queuedata = calypso_dev;

    debug(KERN_INFO, __func__, "Set up request queue\n");
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
    calypso_dev->disk = alloc_disk(CALYPSO_MINOR_CNT);
    if (!calypso_dev->disk)
    {
        debug(KERN_ERR, __func__, "alloc_disk failure\n");
        blk_cleanup_queue(calypso_dev->queue);
        calypso_memory_cleanup(calypso_dev);
        unregister_blkdev((*calypso_major), dev_name);
        return -ENOMEM;
    }
    debug(KERN_INFO, __func__, "Allocated disk\n");

    /* can only have one partition 
    otherwise we need to create a partition table */
    calypso_dev->disk->flags |= GENHD_FL_NO_PART_SCAN;

    /* Setting the major number */
    calypso_dev->disk->major = (*calypso_major);
    /* Setting the first mior number */
    calypso_dev->disk->first_minor = CALYPSO_FIRST_MINOR;
    /* Initializing the device operations */
    calypso_dev->disk->fops = calypso_fops;
    /* Driver-specific own internal data */
    calypso_dev->disk->private_data = calypso_dev;
    calypso_dev->disk->queue = calypso_dev->queue;

    strcat(part_name, dev_name);
    strcat(part_name, "0");
    debug_args(KERN_INFO, __func__, "PARTITION NAME %s\n", part_name);
    sprintf(calypso_dev->disk->disk_name, part_name);

    /* Setting the capacity of the device in its gendisk structure */
    set_capacity(calypso_dev->disk, calypso_dev->capacity);
    debug(KERN_INFO, __func__, "Set disk capacity\n");
 
    /* Adding the disk to the system */
    add_disk(calypso_dev->disk);
    /* Now the disk is "live" */
    debug(KERN_INFO, __func__, "Calypso device initialised\n");

    return 0;
}

void calypso_memory_cleanup(struct calypso_blk_device *calypso_dev)
{
    if (calypso_dev) {
        kfree(calypso_dev);
        calypso_dev = NULL;
    }

    debug(KERN_INFO, __func__, "Calypso memory cleanup done\n");
}
 
void calypso_dev_cleanup(struct calypso_blk_device *calypso_dev,
                        const char *dev_name,
                        int *calypso_major)
{
    debug(KERN_INFO, __func__, "Calypso device cleanup initialized\n");
    
    if (calypso_dev) {
        if (calypso_dev->disk) {
            del_gendisk(calypso_dev->disk);
            put_disk(calypso_dev->disk);
            calypso_dev->disk = NULL;
            debug(KERN_INFO, __func__, "Deleted disk\n");
        }
        if (calypso_dev->queue) {
            blk_cleanup_queue(calypso_dev->queue);
            calypso_dev->queue = NULL;
            debug(KERN_INFO, __func__, "Deleted request queue\n");
        }
    }

    calypso_memory_cleanup(calypso_dev);
    
    unregister_blkdev((*calypso_major), dev_name);

    debug(KERN_INFO, __func__, "Calypso device cleanup finished\n");
}



void calypso_dev_physical_cleanup(struct calypso_blk_device *calypso_dev,
                                const char *dev_name,
                                int *calypso_major)
{
    if (calypso_dev) {
        if (calypso_dev->physical_dev) 
        {
            bdput(calypso_dev->physical_dev);
            calypso_dev->physical_dev = NULL;
            debug(KERN_INFO, __func__, "Put physical device\n");
        }    
        if (calypso_dev->disk) {
            del_gendisk(calypso_dev->disk);
            put_disk(calypso_dev->disk);
            calypso_dev->disk = NULL;
            debug(KERN_INFO, __func__, "Deleted disk\n");
        }
        if (calypso_dev->queue) {
            blk_cleanup_queue(calypso_dev->queue);
            calypso_dev->queue = NULL;
            debug(KERN_INFO, __func__, "Deleted request queue\n");
        }

    }

    unregister_blkdev((*calypso_major), dev_name);

    debug(KERN_INFO, __func__, "Calypso device cleanup finished\n");
}