# Documentation
* Update file position in read/write functions: https://nanxiao.me/en/why-doesnt-linux-device-driver-need-to-update-file-position-in-readwrite-functions/
* followed https://wiki.tldp.org/lkmpg/en/content/ch07/2_6
* Useful guide with schemes: https://olegkutkov.me/2018/03/14/simple-linux-character-device-driver/


# Talking to device files (writes and IOCTLs)
* https://mjmwired.net/kernel/Documentation/ioctl-number.txt

* mechanism for device drivers in the kernel to get the output to send to the device from processes. this is done by opening the device file for output and writing to it, with device_write().

* ioctl (input output control): read -> send information from a process to the kernel: or write -> return informartion to a process
    * three parameters: 
        * file descriptor of the device file
        * #define "ioctl name" __IOX("magic number","command number","argument type")
        * ioctl number, which is usually created by a macro call (_IO for no parameters, _IOR for ioctl with read parameters or copy_to_user, _IOW for an ioctl with write parameters or copy_from_user or _IOWR for an ioctl with both write and read parameters) and encodes:
            * the major device number, or another magic number that defines this function
            * type of the ioctl
            * the command
            * type of parameter
        * a parameter of type long that we can cast to anything to pass any type

* ioctl vs reads/writes: ioctl can be used for reads/writes, but it's overkill; it's better to write operations that cannot be easily classified, or there is not already a system call for that functionality.

* for ioctl we need 3 different files:
    * kernel module: chardev_ioctl/chardev.c
    * shared header between the kernel module and userland: chardev_ioctl/chardev.h
    * userland process: chardev_ioctl/ioctl.c


# Source
## Structures
```    
/*
 * The functions not implemented should be set to NULL
 */ 
struct file_operations {
    struct module *owner;
    loff_t (*llseek) (struct file *, loff_t, int);
    ssize_t (*read) (struct file *, char *, size_t, loff_t *);
    ssize_t (*write) (struct file *, const char *, size_t, loff_t *);
    int (*readdir) (struct file *, void *, filldir_t);
    unsigned int (*poll) (struct file *, struct poll_table_struct *);
    int (*ioctl) (struct inode *, struct file *, unsigned int, unsigned long);
    int (*mmap) (struct file *, struct vm_area_struct *);
    int (*open) (struct inode *, struct file *);
    int (*flush) (struct file *);
    int (*release) (struct inode *, struct file *);
    int (*fsync) (struct file *, struct dentry *, int datasync);
    int (*fasync) (int, struct file *, int);
    int (*lock) (struct file *, int, struct file_lock *);
    ssize_t (*readv) (struct file *, const struct iovec *, unsigned long,
    loff_t *);
    ssize_t (*writev) (struct file *, const struct iovec *, unsigned long,
    loff_t *);
};
```

## Major and minor
* https://embetronicx.com/tutorials/linux/device-drivers/character-device-driver-major-number-and-minor-number/

* Static: if you know in advance which major number you want to start with, you can ask the kernel to assign that specific number. If it's not available, it'll not be created.

* Dynamic: you are telling the kernel that how many device numbers you need (the starting minor number and count) and it will find a starting major number for you, if one is available, of course. Generally preferable to avoid conflicts with other device drivers
    * even though we don't know it in the beginning, we can find the MAJOR number by reading /proc/devices


