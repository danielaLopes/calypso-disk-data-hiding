/*
 *  chardev_impl.h - the header file with the ioctl definitions.
 *
 *  The declarations here have to be in a header file, because
 *  they need to be known both to the kernel module
 *  (in chardev.c) and the process calling ioctl (ioctl.c)
 *  Because it needs a shared header between the kernel module and userland
 */

#ifndef CHARDEV_IMPL_H
#define CHARDEV_IMPL_H

#include <linux/ioctl.h>

#define CLASS_NAME "mychardev"

// see https://tuxthink.blogspot.com/2011/01/creating-ioctl-command.html
#define IOC_MAGIC 'k'

/* 
 * The major device number, dynamically.
 */
extern int dev_major;

/* 
 * Get the n'th byte of the message 
 */
#define IOCTL_GET_NTH_BYTE _IOWR(IOC_MAGIC, 0, int)


#endif