/**
 *  procfs2.c -  create a "file" in /proc
 *
 */

#include <linux/module.h>	/* Specifically, a module */
#include <linux/kernel.h>	/* We're doing kernel work */
#include <linux/proc_fs.h>	/* Necessary because we use the proc fs */
#include <linux/uaccess.h>	/* for copy_from_user */

#define PROCFS_MAX_SIZE		1024
#define PROCFS_NAME 		"helloworld2"

/**
 * This structure hold information about the /proc file
 *
 */
static struct proc_dir_entry *Our_Proc_File;

static struct proc_dir_entry *proc_root;

/**
 * The buffer used to store character for this module
 *
 */
static char procfs_buffer[PROCFS_MAX_SIZE]; // buffer that is going to keep the contents of /proc/helloworld file

/**
 * The size of the buffer
 *
 */
static unsigned long procfs_buffer_size = 0;

/** 
 * This function is called then the /proc/PROCFS_NAME file is read
 *
 * Returns 0 to indicate end of file, otherwise processes will
 * continue to read in an endless loop
 */
static ssize_t 
procfile_read(
		  struct file *file, // per process structure with the opened file details (permission, position, ...)
		  char __user *ubuf, // user space buffer
		  size_t count, // buffer size; max size <= buffer size
		  loff_t *ppos // file offset or current read/write position in the file
		  )          
{

    // preserve their value even when out of scope 
    // and are not initialized again in new scope, 
    // so once finished is set to 1, it stays until being set to 0
    static int finished = 0; 

    if ( finished ) { // finished != 0
		printk(KERN_INFO "procfs_read: END\n");
		finished = 0;
		return 0; // when we finish reading it can return 0 because it read 0 bytes in this call
	}
	
	finished = 1;
	
	printk(KERN_INFO "procfile_read (/proc/%s) called\n", PROCFS_NAME);

    // copy_to_user() returns number of bytes that could not be written or 0 on success
	if(copy_to_user(ubuf, procfs_buffer, procfs_buffer_size)) /* better than memcpy for security reasons
											the kernel has permission to write on any memory page
											so an attacker can make it write in a page it's not supposed
											to if there are no checks that the target page is writable
											by the current process, which copy_to_user does */
		return -EFAULT;
	
	return procfs_buffer_size; // return number of bytes read
    //return 0; // It cannot be 0 because then it will not output what it read in this call
                // It can only return 0 on the call when we no longer have something to read
}

/**
 * This function is called with the /proc file is written
 *
 */
static ssize_t procfile_write(struct file *file, 
			const char __user *ubuf,
			size_t count, 
			loff_t *ppos)
{
	/* get buffer size */
	procfs_buffer_size = count;
	if (procfs_buffer_size > PROCFS_MAX_SIZE ) {
		procfs_buffer_size = PROCFS_MAX_SIZE;
	}
	
	/* write data to the buffer */
	if ( copy_from_user(procfs_buffer, ubuf, procfs_buffer_size) ) {
		return -EFAULT;
	}
	
	return procfs_buffer_size;
}

static const struct file_operations proc_file_fops = {
 .owner = THIS_MODULE,
 .read = procfile_read,
 .write = procfile_write
};

static int proc_init(void)
{
	// this is where /proc/helloworld2 is created
	Our_Proc_File = proc_create(PROCFS_NAME, S_IFREG | S_IRUGO | S_IWUSR, NULL, &proc_file_fops);

	if (Our_Proc_File == NULL) {
		printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
		       PROCFS_NAME);
		return -ENOMEM;
	}

	printk(KERN_INFO "/proc/%s created\n", PROCFS_NAME);	
	return 0;	/* everything is ok */
}

static void proc_cleanup(void)
{
	remove_proc_entry(PROCFS_NAME, proc_root); // without this it was not freeing space right and I could not load the module twice
	printk(KERN_INFO "/proc/%s removed\n", PROCFS_NAME);
}

MODULE_LICENSE("GPL"); 
module_init(proc_init);
module_exit(proc_cleanup);