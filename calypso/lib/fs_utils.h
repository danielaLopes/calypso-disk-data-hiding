#ifndef FS_UTILS_H
#define FS_UTILS_H

#include "ext4/ext4.h"
#include "ext4_fs_utils.h"
#include "virtual_device.h"


#define FILE_PATH "/media/calypso/hello.txt"


unsigned long calypso_get_byte_bitmap(unsigned long *bitmap, int start);

void calypso_set_byte_bitmap(unsigned long *bitmap, unsigned long value, int start);

void calypso_partition_fs_info(struct block_device *physical_dev);

void calypso_get_super_block_physical_dev(struct calypso_blk_device *calypso_dev);

unsigned long calypso_get_physical_block_count(struct calypso_blk_device *calypso_dev);

void calypso_copy_block(struct block_device *physical_dev, unsigned long from_block, unsigned long to_block);

void calypso_print_free_blocks_bitmap(struct block_device *physical_dev, unsigned int nblocks, unsigned long *bitmap);

void calypso_iterate_each_bitmap(const unsigned long *orig_bitmap, unsigned long *res_bitmap, unsigned int nbits, ext4_fsblk_t block_offset);
void calypso_bitmap_helper(const char *ptr, unsigned long *res_bitmap, unsigned int numchars, ext4_fsblk_t block_offset);
void calypso_get_fs_blocks_bitmap(struct calypso_blk_device *calypso_dev);

unsigned int calypso_iterate_partition_blocks_groups(struct block_device *physical_dev);

void calypso_ext4_print_free_blocks(const char *ptr, unsigned int numchars, ext4_fsblk_t block_offset);

int calypso_iterate_bitmap(const unsigned long *src, unsigned int nbits, ext4_fsblk_t block_offset);

static __always_inline ext4_fsblk_t calypso_get_block_no(ext4_fsblk_t block_offset, unsigned int block_no_within_group)
{
	return block_offset + block_no_within_group;
}


#endif