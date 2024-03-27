/*
 * video_ram.c
 * 
 * The video RAM is absent, so the write is not 
 * actually writing, just sending signals into the air
 * 
 * Usage: 
 *  $ echo -n “0123456789” > /dev/vram  # write
 *  $ xxd /dev/vram | less              # read
 *  $ cat /dev/vram                     # just shows the binary content
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <asm/io.h>

/*
 * These values should be obtained from $ sudo cat /proc/iomem
 * However my system map did not show that parameter, so I'm using 
 * another one (PCI Bus 0000:00 )
 */
//#define VRAM_BASE 0x000A0000
//#define VRAM_SIZE 0x00020000
#define VRAM_BASE 0x000A0000
#define VRAM_SIZE 0x00020000

/*
 * https://stackoverflow.com/questions/19100536/what-is-the-use-of-iomem-in-linux-while-writing-device-drivers
 * 
 * Adding and requiring a cookie like __iomem for all I/O accesses is a way
 * to be stricter and avoid programming errors. It's not good to read/write
 * from I/O memory regions with absolute addresses because we are usually using 
 * virtual memory
 */
static void __iomem *vram;  
static dev_t first;
static struct cdev c_dev;
static struct class *cl;

static int my_open(struct inode *i, struct file *f)
{
    return 0;
}
static int my_close(struct inode *i, struct file *f)
{
    return 0;
}
static ssize_t my_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
    int i;
    u8 byte;

    if (*off >= VRAM_SIZE)
    {
        return 0;
    }
    if (*off + len > VRAM_SIZE)
    {
        len = VRAM_SIZE - *off;
    }
    for (i = 0; i < len; i++)
    {
        // function to read with our _iomem cookie called vram (virtual address)
        byte = ioread8((u8 *)vram + *off + i);
        if (copy_to_user(buf + i, &byte, 1))
        {
            return -EFAULT;
        }
    }
    *off += len;

    return len;
}
static ssize_t my_write(
        struct file *f, const char __user *buf, size_t len, loff_t *off)
{
    int i;
    u8 byte;

    if (*off >= VRAM_SIZE)
    {
        return 0;
    }
    if (*off + len > VRAM_SIZE)
    {
        len = VRAM_SIZE - *off;
    }
    for (i = 0; i < len; i++)
    {
        if (copy_from_user(&byte, buf + i, 1))
        {
            return -EFAULT;
        }
        // function to write with our _iomem cookie called vram (virtual address)
        iowrite8(byte, (u8 *)vram + *off + i);
    }
    *off += len;

    return len;
}

static struct file_operations vram_fops =
{
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_close,
    .read = my_read,
    .write = my_write
};

static int __init vram_init(void) /* Constructor */
{
    int ret;
    struct device *dev_ret;

    /* returns a virtual adress corresponding to the start
     * of the requested physical address range
     * 
     * used to obtain the __iomem cookie for reading and writing
     * with ioread8() and iowrite8()
     */
    if ((vram = ioremap(VRAM_BASE, VRAM_SIZE)) == NULL)
    {
        printk(KERN_ERR "Mapping video RAM failed\n");
        return -ENOMEM;
    }
    /* register a range of char device numbers
     * the major number is chosen dynamically
     *
     * int alloc_chrdev_region(dev_t * dev, unsigned baseminor, 
     *                          unsigned count, const char * name);
     * dev is the device; baseminor is the first minor to be used;
     * count is the number of device numbers required, which corresponds
     * to the number of minors
     * 
     * there's also register_chrdev_region if we know in advance
     * which major number we want to start with
     */
    if ((ret = alloc_chrdev_region(&first, 0, 1, "vram")) < 0)
    {
        return ret;
    }
    if (IS_ERR(cl = class_create(THIS_MODULE, "chardrv")))
    {
        unregister_chrdev_region(first, 1);
        return PTR_ERR(cl);
    }
    if (IS_ERR(dev_ret = device_create(cl, NULL, first, NULL, "vram")))
    {
        class_destroy(cl);
        unregister_chrdev_region(first, 1);
        return PTR_ERR(dev_ret);
    }

    cdev_init(&c_dev, &vram_fops);
    if ((ret = cdev_add(&c_dev, first, 1)) < 0)
    {
        device_destroy(cl, first);
        class_destroy(cl);
        unregister_chrdev_region(first, 1);
        return ret;
    }
    return 0;
}

static void __exit vram_exit(void) /* Destructor */
{
    cdev_del(&c_dev);
    device_destroy(cl, first);
    class_destroy(cl);
    unregister_chrdev_region(first, 1);
    iounmap(vram);
}

module_init(vram_init);
module_exit(vram_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Anil Kumar Pugalia <email@sarika-pugs.com>");
MODULE_DESCRIPTION("Video RAM Driver");
