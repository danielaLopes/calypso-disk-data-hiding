/*
 * sysfs3.c - create a "subdir" with multiple "files" in /sys
 *
 * usage:
 *      $ cat /sys/kernel/buffer/buffer_reads
 *      $ cat /sys/kernel/buffer/buffer_writes
 *      $ cat /sys/kernel/buffer/data
 *      $ echo "some new text" | sudo tee -a /sys/kernel/buffer/data
 *      $ cat /sys/kernel/buffer/data
 *      $ cat /sys/kernel/buffer/buffer_reads
 *      $ cat /sys/kernel/buffer/buffer_writes
 */
#include <linux/kernel.h>    /* We're doing kernel work */
#include <linux/module.h>    /* Specifically, a module */
#include <linux/kobject.h>   /* Necessary because we use sysfs */
#include <linux/device.h>

#define sysfs_dir  "buffer"
#define sysfs_data "data"
#define sysfs_reads "buffer_reads"
#define sysfs_writes "buffer_writes"

#define sysfs_max_data_size 1024 /* due to limitations of sysfs, you mustn't go above PAGE_SIZE, 1k is already a *lot* of information for sysfs! */
static char sysfs_buffer[sysfs_max_data_size+1] = "HelloWorld!"; /* an extra byte for the '\0' terminator */
static ssize_t used_buffer_size = 0;
static ssize_t nr_buffer_reads = 0;
static ssize_t nr_buffer_writes = 0;

static ssize_t
buffer_show(struct device *dev,
           struct device_attribute *attr,
           char *buffer)
{
    printk(KERN_INFO "sysfile_read (/sys/kernel/%s/%s) called\n", sysfs_dir, sysfs_data);
    
    /*
     * The only change here is that we now increment nr_buffer_reads (and don't worry about overflows)
     */
    nr_buffer_reads++;
    return sprintf(buffer, "%s", sysfs_buffer);
}

static ssize_t
buffer_store(struct device *dev,
            struct device_attribute *attr,
            const char *buffer,
            size_t count)
{
    used_buffer_size = count > sysfs_max_data_size ? sysfs_max_data_size : count; /* handle MIN(used_buffer_size, count) bytes */
    
    printk(KERN_INFO "sysfile_write (/sys/kernel/%s/%s) called, buffer: %s, count: %ld\n", sysfs_dir, sysfs_data, buffer, count);

    memcpy(sysfs_buffer, buffer, used_buffer_size);
    sysfs_buffer[used_buffer_size] = '\0'; /* this is correct, the buffer is declared to be sysfs_max_data_size+1 bytes! */
    nr_buffer_writes++;

    return used_buffer_size;
}

/*
 * These functions are a bit more complex, as they handle multiple files: both meta_... functions handle reads or writes for
 * buffer_reads and buffer_writes.
 * 
 * Why not just have different functions for both files?
 */
static ssize_t
meta_show(struct device *dev,
          struct device_attribute *attr,
          char *buffer)
{
    ssize_t temp = nr_buffer_reads;
    printk(KERN_INFO "sysfile_read (/sys/kernel/%s/%s) called\n", sysfs_dir, attr->attr.name);

    if (strcmp(attr->attr.name, sysfs_writes) == 0)
    {
        temp = nr_buffer_writes;
    }
    return sprintf(buffer, "%lu", temp);
}

static ssize_t
meta_store(struct device *dev,
           struct device_attribute *attr,
           const char *buffer,
           size_t count)
{
    ssize_t temp = 0;
    sscanf(buffer, "%ld", &temp);
    if (strcmp(attr->attr.name, sysfs_reads) == 0)
    {
        nr_buffer_reads = temp;
    }
    else
    {
        nr_buffer_writes = temp;
    }
    return count;
}

/*
 * It is considered to be a bad idea to make a sysfs file writeable, 
 * so we need to bypass the macro check performed in kernel.h
 */

/* warning! need write-all permission so overriding check */ 
#undef VERIFY_OCTAL_PERMISSIONS
#define VERIFY_OCTAL_PERMISSIONS(perms) (perms)

static DEVICE_ATTR(data, S_IWUGO | S_IRUGO, buffer_show, buffer_store);
/* as both buffer_reads and buffer_writes are handled by the same function, they must be declared as such: */
static DEVICE_ATTR(buffer_reads, S_IWUGO | S_IRUGO, meta_show, meta_store);
static DEVICE_ATTR(buffer_writes, S_IWUGO | S_IRUGO, meta_show, meta_store);


/*
 * The only changes here are the added files to the file list:
 */
static struct attribute *attrs[] = {
    &dev_attr_data.attr,
    &dev_attr_buffer_reads.attr,
    &dev_attr_buffer_writes.attr,
    NULL   /* need to NULL terminate the list of attributes */
};
static struct attribute_group attr_group = {
    .attrs = attrs,
};
static struct kobject *hello_obj = NULL;


int __init sysfs_init(void)
{
    int result = 0;

    /*
     * This is identical to previous example.
     */
    hello_obj = kobject_create_and_add(sysfs_dir, kernel_kobj);
    if (hello_obj == NULL)
    {
        printk (KERN_INFO "%s module failed to load: kobject_create_and_add failed\n", sysfs_data);
        return -ENOMEM;
    }

    result = sysfs_create_group(hello_obj, &attr_group);
    if (result != 0)
    {
        /* creating files failed, thus we must remove the created directory! */
        printk (KERN_INFO "%s module failed to load: sysfs_create_group failed with result %d\n", sysfs_data, result);
        kobject_put(hello_obj);
        return -ENOMEM;
    }

    printk(KERN_INFO "/sys/kernel/%s/%s created\n", sysfs_dir, sysfs_data);
    return result;
}

void __exit sysfs_exit(void)
{
    kobject_put(hello_obj);
    printk (KERN_INFO "/sys/kernel/%s/%s removed\n", sysfs_dir, sysfs_data);
}

module_init(sysfs_init);
module_exit(sysfs_exit);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Freddy Hurkmans");
MODULE_DESCRIPTION("sysfs buffer");