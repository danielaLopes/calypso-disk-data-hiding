/**
 *  procfs_hello.c -  create a "file" in /proc and always make it read "Hello World!"
 *
 */

#include <linux/module.h>	/* Specifically, a module */
#include <linux/kernel.h>	/* We're doing kernel work */
#include <linux/proc_fs.h>	/* Necessary because we use the proc fs */
#include <linux/uaccess.h>	/* for copy_from_user */

#define PROCFS_MAX_SIZE		1024
#define PROCFS_NAME 		"helloworld"
#define BUFSIZE 100

/**
 * This structure hold information about the /proc file
 *
 */
static struct proc_dir_entry *Our_Proc_File;

static struct proc_dir_entry *proc_root;

/** 
 * This function is called then the /proc/PROCFS_NAME file is read
 *
 * Always reads "Hello World!"
 */
static ssize_t 
procfile_read(
		  struct file *file, // per process structure with the opened file details (permission, position, ...)
		  char __user *ubuf, // user space buffer
		  size_t count, // buffer size; max size <= buffer size
		  loff_t *ppos // file offset or current read/write position in the file
		  )          
{

    int read_len = 0;
	char buf[BUFSIZE];
	
	printk(KERN_INFO "procfile_read (/proc/%s) called\n", PROCFS_NAME);

    if (*ppos > 0 || count < BUFSIZE) { // check if it's the first time we call read and if user buffer is bigger than BUFSIZE
		/* we have finished to read, return 0 */
		return 0;
	} else {
		/* fill the buffer, return the buffer size */
		read_len += sprintf(buf, "Hello World!\n"); // returns total number of characters written into buf
	}

    // copy_to_user() returns number of bytes that could not be written or 0 on success
	if(copy_to_user(ubuf, buf, read_len)) /* better than memcpy for security reasons
											the kernel has permission to write on any memory page
											so an attacker can make it write in a page it's not supposed
											to if there are no checks that the target page is writable
											by the current process, which copy_to_user does */
		return -EFAULT;
	
	return read_len; // return number of bytes read
}

static const struct file_operations proc_file_fops = {
 .owner = THIS_MODULE,
 .read = procfile_read
};

static int proc_init(void)
{
    // this is where /proc/helloworld is created
	Our_Proc_File = proc_create(PROCFS_NAME, /*S_IFREG | S_IRUGO*/ 0666, NULL, &proc_file_fops);

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