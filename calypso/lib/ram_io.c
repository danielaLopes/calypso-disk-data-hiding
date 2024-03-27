/*
 *
 */

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/vmalloc.h>

#include "debug.h"
#include "global.h"
#include "ram_io.h"


int calypso_init_ram(struct ram_device **ram_dev, sector_t n_sectors) 
{
	(*ram_dev) = kzalloc(sizeof(struct ram_device), GFP_KERNEL);
	if (!(*ram_dev)) {
		debug(KERN_ERR, __func__, "Could not allocate memory for RAM device\n");
		return -ENOMEM;
	}

    (*ram_dev)->data = kmalloc(n_sectors * DISK_SECTOR_SIZE * sizeof(char), GFP_KERNEL);
    if (!(*ram_dev)->data) {
        debug(KERN_ERR, __func__, "Could not allocate memory for RAM device data\n");
		kfree((*ram_dev));
		(*ram_dev) = NULL;
        return -ENOMEM;
    }

    debug(KERN_INFO, __func__, "Allocated memory for RAM device\n");

	return 0;
}

void calypso_free_ram(struct ram_device *ram_dev) 
{
	if (ram_dev) {

		if (ram_dev->data) {
			kfree(ram_dev->data);
			ram_dev->data = NULL;
		}

		kfree(ram_dev);
		ram_dev = NULL;
	}

	debug(KERN_INFO, __func__, "Freed memory for RAM device\n");
}

void calypso_write_segment_to_ram(char *ram_buf, 
							unsigned long offset_bytes,
							size_t len_bytes, 
                    		u8 *req_buf)
{
	memcpy(ram_buf + offset_bytes, req_buf, len_bytes);
}

void calypso_read_segment_from_ram(char *ram_buf,
							unsigned long offset_bytes, 
							size_t len_bytes,
                    		u8 *req_buf) 
{
	 memcpy(req_buf, ram_buf + offset_bytes, len_bytes);
}