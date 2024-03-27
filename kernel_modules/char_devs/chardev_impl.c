/*
 *  chardev_impl.c - creates character driver with two minor devices
 * mychardev-0 and mychardev-1
 * 
 * Implementation of device functions from file chardev_baseline.c
 * Additionally, there's an ioctl function to print the nth byte
 * of the device file based on the argument given, which must be an int
 * 
 * Usage:
 *      $ tree /sys/devices/virtual/mychardev/
 *      $ ls -l /dev/mychardev-*
 *      $ cat /dev/mychardev-0 
 *      $ head -c29 /dev/mychardev-1 // read 29 bytes from char device
 *      $ echo "Hello from the user" > /dev/mychardev-1
 *      $ cat /dev/mychardev-1
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/uaccess.h> // copy_to_user() and copy_from_user()
                            // these perform additional checks of permissions
                            // and memory regions before data access
#include <linux/fs.h>

#include "chardev_impl.h"

// max Minor devices
#define MAX_DEV 2

#define MAX_DATA_LEN 30

/*
 * -----------------------------------------------------------
 * Device I/O functions declarations
 * -----------------------------------------------------------
 */
static int mychardev_open(struct inode *inode, struct file *file);
static int mychardev_release(struct inode *inode, struct file *file);
static long mychardev_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static ssize_t mychardev_read(struct file *file, char __user *buf, size_t count, loff_t *offset);
static ssize_t mychardev_write(struct file *file, const char __user *buf, size_t count, loff_t *offset);


// initialize file_operations
static const struct file_operations mychardev_fops = {
    .owner      = THIS_MODULE,
    .open       = mychardev_open,
    .release    = mychardev_release,
    .unlocked_ioctl = mychardev_ioctl,
    .read       = mychardev_read,
    .write       = mychardev_write
};

// device data holder, this structure may be extended to hold additional data
struct mychar_device_data {
    struct cdev cdev;
};

// global storage for device Major number
int dev_major = 0;

// sysfs class structure
static struct class *mychardev_class = NULL;

// array of mychar_device_data for
static struct mychar_device_data mychardev_data[MAX_DEV];

/* 
 * The message the device will give when asked 
 */
static char message[MAX_DATA_LEN];

/*
 * Set correct permissions to the character device
 * Allows every user to read and write
 */
static int mychardev_uevent(struct device *dev, struct kobj_uevent_env *env)
{
    add_uevent_var(env, "DEVMODE=%#o", 0666);
    return 0;
}

static int __init mychardev_init(void)
{
    int err, i;
    dev_t dev;

    // allocate chardev region and assign Major number
    err = alloc_chrdev_region(&dev, 0, MAX_DEV, CLASS_NAME);

    dev_major = MAJOR(dev);

    printk("MAJOR NUMBER: %d\n", dev_major);

    // create sysfs class
    // creates directories in /sys/devices/virtual/mychardev/
    mychardev_class = class_create(THIS_MODULE, CLASS_NAME);
    mychardev_class->dev_uevent = mychardev_uevent;

    // Create necessary number of the devices
    for (i = 0; i < MAX_DEV; i++) {
        // init new device
        cdev_init(&mychardev_data[i].cdev, &mychardev_fops);
        mychardev_data[i].cdev.owner = THIS_MODULE;

        // add device to the system where "i" is a Minor number of the new device
        cdev_add(&mychardev_data[i].cdev, MKDEV(dev_major, i), 1);

        // create device node /dev/mychardev-x where "x" is "i", equal to the Minor number
        device_create(mychardev_class, NULL, MKDEV(dev_major, i), NULL, "mychardev-%d", i);
    }

    return 0;
}

/*
 * Destroy character drive
 */
static void __exit mychardev_exit(void)
{
    int i;

    for (i = 0; i < MAX_DEV; i++) {
        device_destroy(mychardev_class, MKDEV(dev_major, i));
    }

    class_unregister(mychardev_class);
    class_destroy(mychardev_class);

    unregister_chrdev_region(MKDEV(dev_major, 0), MINORMASK);
}


/*
 * -----------------------------------------------------------
 * Device I/O functions definitions
 * -----------------------------------------------------------
 */
static int mychardev_open(struct inode *inode, struct file *file)
{   
    printk("MYCHARDEV: Device open\n");
    return 0;
}

static int mychardev_release(struct inode *inode, struct file *file)
{
    printk("MYCHARDEV: Device close\n");
    return 0;
}

/*
 * This is used to pass some CMD as the number and some optional data as ARG
 * For that, we need to define magic numbers used as CMD and ARG in a 
 * separate header file, shared between driver code and user application code
 * 
 * The implementation of this function can be a simple switch routine
 * where we do something depending on the sent CMD
 */
static long mychardev_ioctl(struct file *file, unsigned int ioctl_cmd, unsigned long ioctl_param)
{
    printk("MYCHARDEV: Device ioctl\n");
    printk("ioctl_param %d\n", ioctl_param);

    /* 
    * Switch according to the ioctl called 
    */
    switch (ioctl_cmd) {
        
        case IOCTL_GET_NTH_BYTE:
            /* 
            * This ioctl is both input (ioctl_param) and 
            * output (the return value of this function) 
            */
            return message[ioctl_param];
            break;
    }

    return 0;
}

/*
 * Needs to return 0, otherwise it will loop endlessly
 * For this we can use the offset pointer, as
 * suggested in https://stackoverflow.com/questions/12124628/endlessly-looping-when-reading-from-character-device
 */
static ssize_t mychardev_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
    //uint8_t *data = "Hello from the kernel world!\n";
    //size_t datalen = strlen(message);

    /*
     * Equivalent to: 
     * if (count < (MAX_DATA_LEN-(*offset))) {
     *      bytes = count;
     * } else {
     *      bytes = MAX_DATA_LEN-(*offset);
     * }
     * 
     * It's very important to check how many bytes users want to read.
     * If it exceeds the prepared data's actual size, the user can read
     * the kernel stack, which is very dangerous for system security
     */
    ssize_t bytes = count < (MAX_DATA_LEN-(*offset)) ? count : (MAX_DATA_LEN-(*offset));

    printk("MYCHARDEV: Device read\n");
    // MINOR(file->f_path.dentry->d_inode->i_rdev) to get the minor number from the file inode
    printk("Reading device: %d\n", MINOR(file->f_path.dentry->d_inode->i_rdev));

    //if (copy_to_user(buf, data, count)) {
    if (copy_to_user(buf, message, bytes)) {
        return -EFAULT;
    }

    (*offset) += bytes;

    return bytes;
}

/*
 * Note: if not implemented, it will print the message continuously
 */
static ssize_t mychardev_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
    size_t maxdatalen = MAX_DATA_LEN, ncopied;
    //uint8_t databuf[MAX_DATA_LEN];

    printk("MYCHARDEV: Device write\n");
    // MINOR(file->f_path.dentry->d_inode->i_rdev) to get the minor number from the file inode
    printk("Writing device: %d\n", MINOR(file->f_path.dentry->d_inode->i_rdev));

    /*
     * it's also very important to verify how many bytes sending users and 
     * how many bytes we can accept
     * 
     */
    if (count < maxdatalen) {
        maxdatalen = count;
    }

    /*
     * copy_from_user returns the number of bytes that could not be copied
     * on success, it returns 0
     */
    //ncopied = copy_from_user(databuf, buf, maxdatalen);
    ncopied = copy_from_user(message, buf, maxdatalen);

    if (ncopied == 0) {
        printk("Copied %zd bytes from the user\n", maxdatalen);
    } else {
        printk("Could't copy %zd bytes from the user\n", ncopied);
    }

    // pads with "0" bytes
    //databuf[maxdatalen] = 0;
    message[maxdatalen] = 0;

    //printk("Data from the user: %s\n", databuf);
    printk("Data from the user: %s\n", message);

    return count;
}

MODULE_LICENSE("GPL");
//MODULE_AUTHOR("Oleg Kutkov <elenbert@gmail.com>");

module_init(mychardev_init);
module_exit(mychardev_exit);