/*
 * sysfs1.c - create a "subdir" with a readonly "file" in /sys
 * 
 * to read the file: $ cat /sys/kernel/hello/helloworld
 */
#include <linux/kernel.h>    /* We're doing kernel work */
#include <linux/module.h>    /* Specifically, a module */
#include <linux/kobject.h>   /* Necessary because we use sysfs */
#include <linux/device.h>

#define sysfs_dir  "hello"
#define sysfs_file "helloworld"

/* equivalent to implementing read callback */
static ssize_t
sysfs_show(struct device *dev,
           struct device_attribute *attr,
           char *buffer)
{
    printk(KERN_INFO "sysfile_read (/sys/kernel/%s/%s) called\n", sysfs_dir, sysfs_file);
    
    /*
     * In sysfs you are supposed to give little information and all in one go.
     * A small quote from https://www.kernel.org/doc/Documentation/filesystems/sysfs.txt:
     *    Mixing Types, expressing multiple lines of data, and doing fancy formatting
     *    of data is heavily frowned upon.
     *
     * In contrary to both procfs and device nodes, the show function will only
     * be called once, so there's no mechanism needed to show we're done.
     *
     * The return value of this function is the number of bytes we've written into buffer,
     * which is exactly what sprintf returns.
     */
    return sprintf(buffer, "HelloWorld!\n");
}


/* 
 * We must define some structures to hold information about the sysfs file:
 *
 * What we need to do is something along the lines of:
 *   static struct device_attribute dev_attr_helloworld = {
 *       .attr = {
 *           .name = "helloworld",
 *           .mode = S_IRUGO,
 *       },
 *       .show = sysfs_show,
 *       .store = NULL,
 *   };
 *
 * The following is a shortcut that does exactly that. Beware, the filename
 * that will be created (helloworld) must be without quotes! This is because
 * the name helloworld is also used to form the struct name dev_attr_helloworld.
 */

/*
 * #define DEVICE_ATTR(name,mode,show,store)
 * show and store are callbacks for read and write
 * 
 * Declaration of attributes for sysfs file helloworld,
 * which will actually be called dev_attr_helloworld
 * permissions: S_IRUGO: everyone can read the file
 * sysfs_show: callback to when file is read
 * NULL for no callback for when the file is read
 */
static DEVICE_ATTR(helloworld, S_IRUGO, sysfs_show, NULL);


/*
 * Then we must define a list with device attributes (such as we just created)
 * that we support in this module. In our case it's only one attribute. The list
 * with device attributes must be NULL terminated!
 *
 * Please note: while we gave the name 'helloworld' in the DEVICE_ATTR call,
 * it has actually made a 'dev_attr_helloworld' for us!
 */
static struct attribute *attrs[] = {
    &dev_attr_helloworld.attr,
    NULL   /* need to NULL terminate the list of attributes */
};

/*
 * We're nearly there. We now have a list of supported attributes, this must
 * be filled in in an attribute_group structure:
 */
static struct attribute_group attr_group = {
    .attrs = attrs,
};

/* 
 * And finally: we need a kobject to keep our object creation result in. In
 * sysfs_exit this result is used to delete our created kobject
 */ 
static struct kobject *hello_obj = NULL;

int __init sysfs_init(void)
{
    int result = 0;

    /*
     * First we must create our kobject, which is a directory in /sys/...
     * Since we use kernel_kobj as parameter, the directory is placed in /sys/kernel.
     * Other options are:
     * - mm_kobj          for /sys/kernel/mm/...
     * - hypervisor_kobj  for /sys/hypervisor/...
     * - power_kobj       for /sys/power/...   (which doesn't seem to work on my machine)
     * - firmware_kobj    for /sys/firmware/...
     * Please take a look at kobject.h for more details.
     */
    hello_obj = kobject_create_and_add(sysfs_dir, kernel_kobj);
    if (hello_obj == NULL)
    {
        printk (KERN_INFO "%s module failed to load: kobject_create_and_add failed\n", sysfs_file);
        return -ENOMEM;
    }

    /* now we create all files in the created subdirectory */
    result = sysfs_create_group(hello_obj, &attr_group);
    if (result != 0)
    {
        /* creating files failed, thus we must remove the created directory! */
        printk (KERN_INFO "%s module failed to load: sysfs_create_group failed with result %d\n", sysfs_file, result);
        kobject_put(hello_obj);
        return -ENOMEM;
    }

    printk(KERN_INFO "/sys/kernel/%s/%s created\n", sysfs_dir, sysfs_file);
    return result;
}

void __exit sysfs_exit(void)
{
    kobject_put(hello_obj);
    printk (KERN_INFO "/sys/kernel/%s/%s removed\n", sysfs_dir, sysfs_file);
}

module_init(sysfs_init);
module_exit(sysfs_exit);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Daniela Lopes");
MODULE_DESCRIPTION("sysfs helloworld");