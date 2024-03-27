#ifndef PHYSICAL_DEVICE_H
#define PHYSICAL_DEVICE_H

#include "virtual_device.h"


#define PHYSICAL_DEV_MODE (FMODE_READ | FMODE_WRITE)


struct block_device *calypso_open_disk_simple(char *dev_path);

struct block_device *calypso_open_physical_disk(char *dev_path, 
                                    struct calypso_blk_device *calypso_dev);

void calypso_close_disk_simple(struct block_device *bdev);

void calypso_close_physical_disk(struct block_device *bdev);


#endif