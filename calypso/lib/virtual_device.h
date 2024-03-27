#ifndef VIRTUAL_DEVICE_H
#define VIRTUAL_DEVICE_H

#include <linux/types.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>

#include "ext4/ext4.h"
#include "block_encryption.h"


#define CALYPSO_FIRST_MINOR 0
#define CALYPSO_MINOR_CNT 1

#define __REQ_CALYPSO   29
#define REQ_CALYPSO		(1ULL << __REQ_CALYPSO)

/* 
 * The internal structure representation of our device
 */
struct calypso_blk_device
{
    int major;
    
    /* Size of the device (in sectors) */
    sector_t capacity;

    /* For exclusive access to our request queue */
    spinlock_t lock;
//     /* How many users */
//     //short users;
//     /* */
    struct blk_mq_tag_set tag_set;	
    /* Our request queue */
    struct request_queue *queue;
    /* This is kernel's representation of an individual disk device */
    struct gendisk *disk;

    struct block_device *physical_dev;
    /* structures to retrieve info on physical block device fs */
    struct super_block *physical_super_block;
    struct ext4_sb_info *physical_ext4_sb_info;
    struct ext4_super_block *physical_ext4_sb;
    ext4_group_t physical_nr_groups;

    // TODO check if this can be int instead of long
    unsigned long metadata_nr_blocks;
    unsigned long virtual_nr_blocks;
    unsigned long physical_nr_blocks;
    unsigned long high_entropy_blocks; /* amount of usable blocks in the native disk */ 

    unsigned long bitmap_data_len;
    unsigned long mappings_data_len;

    /* Bitmaps */
    /* The bits set in this are all the unused blocks that have high entropy, 
    thus can be used to hide information. These bits are to be unset when the corresponding 
    passes from unused to used */
    unsigned long *high_entropy_blocks_bitmap;
    unsigned long *physical_blocks_bitmap;

    /* Amount of usable free blocks on the native disk due to having high entropy */
    unsigned long free_high_entropy_blocks;

    /* We only need to store this in memory since when we are executing
    Calypso after the first time, we need to use the same blocks that
    were assigned the first time, which we can obtain when we retrieve metadata */
    unsigned long *metadata_to_physical_block_mapping;

    /* We are only going to persist the virtual to physical mappings
    since we can recover both from this one and it is the smaller one */
    unsigned long *virtual_to_physical_block_mapping;
    unsigned long *physical_to_virtual_block_mapping;

    sector_t first_physical_sector;
    sector_t last_physical_sector;

    /* In case we need to copy data to another block due to overwrites */
    unsigned long from_physical_block_pending_copy;
    unsigned long to_physical_block_pending_copy;
    struct bio *pending_bio;
    struct page *pending_page;
    char *pending_data;

    /**
     * Crypto data
     */
    struct crypto_shash *sym_enc_tfm;

    struct calypso_skcipher_def *cipher;
};

int calypso_dev_init_bitmaps(struct calypso_blk_device *calypso_dev);
void calypso_dev_cleanup_bitmaps(struct calypso_blk_device *calypso_dev);

int calypso_dev_init_mappings(struct calypso_blk_device *calypso_dev);
void calypso_dev_cleanup_mappings(struct calypso_blk_device *calypso_dev);

void calypso_set_physical_partition_sector_ranges(struct calypso_blk_device *calypso_dev);

unsigned long calypso_get_block_nr_from_sector(sector_t cur_sector, sector_t first_sector);
sector_t calypso_get_sector_nr_from_block(unsigned long block, unsigned long offset);

unsigned long calypso_get_free_blocks_group(unsigned long *bitmap, unsigned long last_block, unsigned int nblocks);
int calypso_get_next_free_block(unsigned long *bitmap, unsigned long last_block, unsigned long *free_block);

void calypso_set_bit(unsigned long *bitmap, unsigned long start);
void calypso_clear_bit(unsigned long *bitmap, unsigned long start);
void calypso_update_bitmaps(unsigned long *physical_blocks_bitmap, unsigned long *high_entropy_blocks_bitmap, unsigned long start);

int calypso_dev_init(struct calypso_blk_device **calypso_dev);

int calypso_dev_setup(struct calypso_blk_device *calypso_dev, 
                    struct block_device_operations *calypso_fops, 
                    struct blk_mq_ops *calypso_fops_req,
                    const char *dev_name,
                    int *calypso_major);

int calypso_dev_no_queue_setup(struct calypso_blk_device *calypso_dev, 
                    struct block_device_operations *calypso_fops,
                    const char *dev_name,
                    int *calypso_major,
                    blk_qc_t (*calypso_make_request)(struct request_queue *q, struct bio *bio));

void calypso_memory_cleanup(struct calypso_blk_device *calypso_dev);

void calypso_dev_cleanup(struct calypso_blk_device *calypso_dev,
                        const char *dev_name,
                        int *calypso_major);

void calypso_dev_physical_cleanup(struct calypso_blk_device *calypso_dev,
                        const char *dev_name,
                        int *calypso_major);


#endif