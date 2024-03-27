#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/bitmap.h>
#include <linux/fs.h>

#include "persistence.h"
#include "debug.h"
#include "file_io.h"
#include "fs_utils.h"


void calypso_write_mappings_to_file(struct file *metadata_file, struct calypso_blk_device *calypso_dev) {
    unsigned long virtual;
    char physical_str[8+1];
    for (virtual = 0; virtual < calypso_dev->virtual_nr_blocks; virtual++)
    {
        snprintf(physical_str, 8+1, "%.8lx", calypso_dev->virtual_to_physical_block_mapping[virtual]);
        calypso_write_file(metadata_file, physical_str, 8);
    }
}

void calypso_read_mappings(struct file *metadata_file, struct calypso_blk_device *calypso_dev)
{
    unsigned long virtual = 0;
    char physical_str[8];
    unsigned long physical;
    loff_t offset = 0;

    while (virtual < calypso_dev->virtual_nr_blocks)
    {
        calypso_read_file_with_offset(metadata_file, offset, physical_str, 8);
        offset += 8;
        kstrtoul(physical_str, 16, &physical);
        /* We only set the mappings if they are not the default value */
        if (physical < MAX_UNSIGNED_LONG)
        {
            debug(KERN_DEBUG, __func__, "MAPPED!\n");
            debug_args(KERN_DEBUG, __func__, "virtual: %lu; physical: %lu\n", virtual, physical);
            calypso_dev->virtual_to_physical_block_mapping[virtual] = physical;
            calypso_dev->physical_to_virtual_block_mapping[physical] = virtual;
        }
        virtual++;
    }
}

void calypso_write_bitmap_to_file(struct file *metadata_file, struct calypso_blk_device *calypso_dev) 
{
    int res = 0;

    int remainder = calypso_dev->physical_nr_blocks % 8;
    loff_t bitmap_data_len = calypso_dev->physical_nr_blocks / 8 + remainder;

    u32 *buf = kzalloc(bitmap_data_len, GFP_KERNEL);
    bitmap_to_arr32(buf, calypso_dev->physical_blocks_bitmap, calypso_dev->physical_nr_blocks);
    calypso_write_file(metadata_file, (char *)buf, bitmap_data_len);

    kfree(buf);
}

int calypso_read_bitmap(char *bitmap_data, loff_t bitmap_len, struct calypso_blk_device *calypso_dev) 
{
    unsigned long *prev_bitmap, *res_bitmap;
    unsigned long i, long_val;
    unsigned int res_rs, res_re;

    res_bitmap = bitmap_alloc(calypso_dev->physical_nr_blocks, GFP_KERNEL);
    if (!res_bitmap)
    {
        debug(KERN_ERR, __func__, "Could not allocate memory for resulting bitmap\n");
        return -ENOMEM;
    }

    /* Build the bitmap with the previous information */
    // TODO check if we actually need to zero the bitmap or if it can be a simple alloc
    prev_bitmap = bitmap_zalloc(calypso_dev->physical_nr_blocks, GFP_KERNEL);
    if (!prev_bitmap)
    {
        debug(KERN_ERR, __func__, "Could not allocate memory for previous bitmap\n");
        return -ENOMEM;
    }

    bitmap_from_arr32(prev_bitmap, (u32 *)bitmap_data, calypso_dev->physical_nr_blocks);

    if (bitmap_equal(prev_bitmap, calypso_dev->physical_blocks_bitmap, calypso_dev->physical_nr_blocks))
    {
        debug(KERN_DEBUG, __func__, "BITMAPS ARE EQUAL!\n");
    }
    else
        debug(KERN_DEBUG, __func__, "BITMAPS ARE NOT EQUAL!\n");

    /* Check differences between previous and updated bitmap */
    bitmap_xor(res_bitmap, prev_bitmap, calypso_dev->physical_blocks_bitmap, calypso_dev->physical_nr_blocks);

    bitmap_for_each_set_region(res_bitmap, res_rs, res_re, 0, calypso_dev->physical_nr_blocks)
	{
        // TODO check if these blocks were mapped by Calypso, only those ones matter
		debug_args(KERN_DEBUG, __func__, "***** THE FOLLOWING REGION MIGHT BE CORRUPTED ***** rs : %u; re: %u;\n", res_rs, res_re);
	}

    bitmap_free(res_bitmap);
    bitmap_free(prev_bitmap);

    return 0;
}

void calypso_persist_bitmap(struct calypso_blk_device *calypso_dev) 
{
    struct file *metadata_file = calypso_open_file(CALYPSO_BITMAP_FILE_PATH, O_CREAT|O_RDWR, 0755);
    calypso_write_bitmap_to_file(metadata_file, calypso_dev);
    calypso_close_file(metadata_file);
}

void calypso_persist_mappings(struct calypso_blk_device *calypso_dev) 
{
    struct file *metadata_file = calypso_open_file(CALYPSO_MAPPINGS_FILE_PATH, O_CREAT|O_RDWR, 0755);
    calypso_write_mappings_to_file(metadata_file, calypso_dev);
    calypso_close_file(metadata_file);
}

void calypso_retrieve_mappings(struct calypso_blk_device *calypso_dev) 
{
    int ret;
    loff_t data_len;

    struct file *metadata_file = calypso_open_file(CALYPSO_MAPPINGS_FILE_PATH, O_CREAT|O_RDWR, 0755);
    data_len = calypso_get_file_size(metadata_file);
    /* if metadata file is empty, it means we have nothing to retrieve */
    if (data_len == 0)
        return -1;

    calypso_read_mappings(metadata_file, calypso_dev);

    calypso_close_file(metadata_file);
}

void calypso_retrieve_bitmap(struct calypso_blk_device *calypso_dev) 
{
    int ret;
    loff_t data_len;

    struct file *metadata_file = calypso_open_file(CALYPSO_BITMAP_FILE_PATH, O_CREAT|O_RDWR, 0755);
    data_len = calypso_get_file_size(metadata_file);
    /* if metadata file is empty, it means we have nothing to retrieve */
    if (data_len == 0)
        return -1;

    char *data = kzalloc(data_len, GFP_KERNEL);
    calypso_read_file_with_offset(metadata_file, 0, data, data_len);

    calypso_read_bitmap(data, data_len, calypso_dev);

    calypso_close_file(metadata_file);
    kfree(data);
}

/* set bits of blocks currently in use by Calypso in bitmap */
void calypso_set_mapped_bits(struct calypso_blk_device *calypso_dev)
{
    size_t i;
    unsigned long physical_block;

    for (i = 0; i < calypso_dev->virtual_nr_blocks; i++)
    {
        physical_block = calypso_dev->virtual_to_physical_block_mapping[i];
        /* block is mapped */
        if (physical_block < MAX_UNSIGNED_LONG)
        {
            debug_args(KERN_DEBUG, __func__, "Set physical block %lu in use by Calypso in bitmap\n", physical_block);
            calypso_set_bit(calypso_dev->physical_blocks_bitmap, physical_block);
        }
    }
}

/* 
 * Write to file
 */
void calypso_persist_metadata(struct calypso_blk_device *calypso_dev) 
{
    calypso_persist_mappings(calypso_dev);
    calypso_persist_bitmap(calypso_dev);
}

/*
 * Read from file
 */
void calypso_retrieve_metadata(struct calypso_blk_device *calypso_dev)
{
    calypso_retrieve_mappings(calypso_dev);
    //calypso_set_mapped_bits(calypso_dev);
    calypso_retrieve_bitmap(calypso_dev);
    // ** TODO this does not make sense!!!!!!!!! to be done only after checking if the bitmaps are equivalent, we should set the bits before comparing because the persisted bitmap should contain these changes
    /* It makes sense to only do this here because the bitmap retrieved from the file system
    does not contain the Calypso mappings set */
    calypso_set_mapped_bits(calypso_dev);
}