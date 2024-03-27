/* 
 * Calypso block device driver
 * Block driver implementation responsible for reading 
 * and writing data to the shadow partition
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/genhd.h> /* allow partitions and gendisk */
#include <linux/blkdev.h>
#include <linux/errno.h>
#include <linux/blk-mq.h>
/* ioctl */
#include <linux/hdreg.h>
#include <linux/blkpg.h>
/* remap io requests */
#include <trace/events/block.h>
#include <linux/kthread.h>
 
#include "../../lib/global.h"
#include "../../lib/debug.h"
#include "../../lib/disk_io.h"
#include "../../lib/file_io.h"
#include "../../lib/ram_io.h"
#include "../../lib/virtual_device.h"
#include "../../lib/physical_device.h"
#include "../../lib/fs_utils.h"

  
/* 
 * This is the registration and initialization section of the block device
 * driver
 */
static int __init calypso_init(void)
{
    int ret = 0;

	struct block_device *physical_dev = blkdev_get_by_path(PHYSICAL_DISK_NAME, FMODE_READ | FMODE_WRITE, THIS_MODULE);
	if (IS_ERR(physical_dev)) {
		debug(KERN_ERR, __func__, "blkdev_get_by_path()\n");
        return -ENOENT;
	}

	calypso_partition_fs_info(physical_dev);
	
	calypso_iterate_partition_blocks_groups(physical_dev);

    debug(KERN_INFO, __func__, "Initialized Parser for ext4 partition\n");

    return ret;
}
/*
 * This is the unregistration and uninitialization section of the ram block
 * device driver
 */
static void __exit calypso_cleanup(void)
{
    debug(KERN_DEBUG, __func__, "Parser for ext4 partition cleanup\n");
}
 
module_init(calypso_init);
module_exit(calypso_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Daniela Lopes");
MODULE_DESCRIPTION("Calypso Driver");