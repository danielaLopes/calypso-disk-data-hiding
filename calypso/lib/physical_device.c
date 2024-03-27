#include <linux/kernel.h>
#include <linux/blkdev.h>
#include <linux/genhd.h>

#include "debug.h"
#include "physical_device.h"


struct block_device *calypso_open_disk_simple(char *dev_path)
{
	struct block_device *bdev;

	debug_args(KERN_INFO, __func__, "Before open physical device: %s\n", dev_path);

	bdev = blkdev_get_by_path(dev_path, FMODE_READ | FMODE_WRITE | FMODE_EXCL, THIS_MODULE);
	if (IS_ERR(bdev)) {
		debug(KERN_ERR, __func__, "blkdev_get_by_path\n");
		return NULL;
	}

	return bdev;
}

struct block_device *calypso_open_physical_disk(char *dev_path, struct calypso_blk_device *calypso_dev)
{
	struct block_device *bdev;

	debug_args(KERN_INFO, __func__, "Before open physical device: %s\n", dev_path);

	//bdev = blkdev_get_by_path(dev_path, FMODE_READ | FMODE_WRITE | FMODE_EXCL, calypso_dev);
    bdev = blkdev_get_by_path(dev_path, FMODE_READ | FMODE_WRITE, calypso_dev);
	if (IS_ERR(bdev)) {
		debug(KERN_ERR, __func__, "blkdev_get_by_path()\n");
		return NULL;
	}

	return bdev;
}

void calypso_close_disk_simple(struct block_device *bdev)
{
	blkdev_put(bdev, FMODE_READ | FMODE_WRITE | FMODE_EXCL);
}

void calypso_close_physical_disk(struct block_device *physical_dev)
{
	//blkdev_put(physical_dev, FMODE_READ | FMODE_WRITE | FMODE_EXCL);
    if (physical_dev)
    {
        blkdev_put(physical_dev, PHYSICAL_DEV_MODE);
        physical_dev = NULL; 
        debug(KERN_INFO, __func__, "Closed physical disk\n");
    }
}