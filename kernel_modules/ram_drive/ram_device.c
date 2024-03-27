/*
 * Provides disk operations API such as init/cleanup 
 * and read/write, using existing RAM operations such
 * as vmalloc/vfree and memcpy
 */

#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
 
#include "ram_device.h"
#include "partition.h"

#define RB_DEVICE_SIZE 1024 /* sectors */
/* So, total device size = 1024 * 512 bytes = 512 KiB */
 
/* Array where the disk stores its data */
static u8 *dev_data;
 
int ramdevice_init(void)
{
    dev_data = vmalloc(RB_DEVICE_SIZE * RB_SECTOR_SIZE);
    if (dev_data == NULL)
        return -ENOMEM;
    /* Setup its partition table */
    copy_mbr_n_br(dev_data);
    return RB_DEVICE_SIZE;
}
 
void ramdevice_cleanup(void)
{
    vfree(dev_data);
}
 
// void ramdevice_write(unsigned long offset_bytes, u8 *buffer, unsigned long n_bytes)
// {
//     memcpy(dev_data + offset_bytes, buffer, n_bytes);
// }
// void ramdevice_read(unsigned long offset_bytes, u8 *buffer, unsigned long n_bytes)
// {
//     memcpy(buffer, dev_data + offset_bytes, n_bytes);
// }
void ramdevice_write(sector_t sector_off, u8 *buffer, unsigned int sectors)
{
	memcpy(dev_data + sector_off * RB_SECTOR_SIZE, buffer,
		sectors * RB_SECTOR_SIZE);
}
void ramdevice_read(sector_t sector_off, u8 *buffer, unsigned int sectors)
{
	memcpy(buffer, dev_data + sector_off * RB_SECTOR_SIZE,
		sectors * RB_SECTOR_SIZE);
}