/*
 * gcc -Wall access_blk_dev.c -o access_blk_dev
 * ./access_blk_dev
 * 
 * $ sudo fdisk -l /dev/sda5
 * > 84690944 sectors
 * 
 * $ ./access_blk_dev
 * > 84690944 sectors
 */
#include <errno.h> /* For errno */
#include <string.h> /* For strerror() */
#include <sys/ioctl.h> /* For ioctl() */
#include <linux/fs.h> /* For BLKGETSIZE64 */
#include <stdio.h> /* for printf */
#include <fcntl.h> /* for open */


#include "access_blk_dev.h"


typedef unsigned long long byte8_t;

static byte8_t size;

int print_blk_dev_info(char *blk_dev_name) {
    
    int sfs_handle = open(blk_dev_name, O_RDWR);
    if (sfs_handle == -1)
    {
        fprintf(stderr, "Error opening %s: %s\n", blk_dev_name, strerror(errno));
        return 2;
    }
    if (ioctl(sfs_handle, BLKGETSIZE64, &size) == -1)
    {
        fprintf(stderr, "Error getting size of %s: %s\n", blk_dev_name, strerror(errno));
        return 3;
    }

    printf("%s size is %lld blocks", blk_dev_name, size / FS_BLOCK_SIZE);

    return 0;
}

int main() {

    int ret = print_blk_dev_info(BLK_DEV_NAME);

    return ret;
}