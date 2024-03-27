/* 
 *  procfs_inode.c -  create a "file" in /proc, use the file_operation way
 *  		to manage the file.
 * Using inodes allows to use advanced functions, like permissions
 */
 
#include <linux/kernel.h>	/* We're doing kernel work */
#include <linux/module.h>	/* Specifically, a module */
#include <linux/proc_fs.h>	/* Necessary because we use proc fs */
#include <linux/sched.h>    /* defines the structure of every process that runs in the system, including current */
#include <linux/uaccess.h>	/* for copy_*_user, get_user and put_user */
#include <linux/cred.h>	    /* credentials, including task credentials */
#include "linux/internal.h" /* defines proc_dir_entry */
//#include <asm-generic/internal.h> /* defines proc_dir_entry */

#define PROCFS_NAME     	"buffer2k"
#define PROCFS_MAX_SIZE 	2048

/**
 * The buffer (2k) for this module
 *
 */
static char procfs_buffer[PROCFS_MAX_SIZE];

/**
 * The size of the data hold in the buffer
 *
 */
static unsigned long procfs_buffer_size = 0;

/**
 * The structure keeping information about the /proc file
 *
 */
static struct proc_dir_entry *proc;

struct proc_dir_entry proc_root;  // it is declared as extern in linux/internal.h, 
                                // which does not allocate memory for it and produces
                                // a compilation error during linkage, so we need to 
                                // declare it here without any modifier to allocate the memory
/**
 * This funtion is called when the /proc file is read
 *
 */
static ssize_t procfs_read(struct file *filp,	/* see include/linux/fs.h   */
			     char *buffer,	/* buffer to fill with data */
			     size_t length,	/* length of the buffer     */
			     loff_t * offset)
{
	static int finished = 0;

	printk(KERN_INFO "procfs_read finished: %d END\n", finished);

	/* 
	 * We return 0 to indicate end of file, that we have
	 * no more information. Otherwise, processes will
	 * continue to read from us in an endless loop. 
	 */
	if ( finished ) {
		printk(KERN_INFO "procfs_read: END\n");
		finished = 0;
		return 0;
	}
	
	// to avoid bug when we read 0 bytes
	// because if it does not call this function again after reading some bytes
	// it will never enter the end condition when finished is set to 0
	if (procfs_buffer_size > 0)
		finished = 1;
		
	/* 
	 * We use put_to_user() to copy the string from the kernel's
	 * memory segment to the memory segment of the process
	 * that called us. get_from_user() used for the reverse. 
	 */
	if ( copy_to_user(buffer, procfs_buffer, procfs_buffer_size) ) {
		return -EFAULT;
	}

	printk(KERN_INFO "procfs_read: read %lu bytes\n", procfs_buffer_size);

	return procfs_buffer_size;	/* Return the number of bytes "read" */
}

/*
 * This function is called when /proc is written
 */
static ssize_t
procfs_write(struct file *file, const char *buffer, size_t len, loff_t * off)
{
	if ( len > PROCFS_MAX_SIZE )	{
		procfs_buffer_size = PROCFS_MAX_SIZE;
	}
	else	{
		procfs_buffer_size = len;
	}
	
	if ( copy_from_user(procfs_buffer, buffer, procfs_buffer_size) ) {
		return -EFAULT;
	}

	printk(KERN_INFO "procfs_write: write %lu bytes\n", procfs_buffer_size);
	
	return procfs_buffer_size;
}

/* 
 * Called whenever a process tries to do something with the /proc/... file
 * and decides whether to allow access or not
 * Only based on the operation and uid of the current user
 * as available in current, a pointer to a structure which includes 
 * information on the currently running process
 * 
 * But it could be based on other things, such as what other processes
 * are doing with the same file, time of day or the last input we received
 * 
 * This function decides whether to allow an operation
 * (return zero) or not allow it (return a non-zero
 * which indicates why it is not allowed).
 *
 * The operation can be one of the following values:
 * 0 - Execute (run the "file" - meaningless in our case)
 * 2 - Write (input to the kernel module)
 * 4 - Read (output from the kernel module)
 *
 * This is the real function that checks file
 * permissions. The permissions returned by ls -l are
 * for referece only, and can be overridden here.
 */

static int module_permission(struct inode *inode, int op)
{   
    // current process explained: https://tuxthink.blogspot.com/2011/04/current-process.html
    // https://www.humblec.com/retrieving-current-processtask_struct-in-linux-kernel/
    // processes are defined by a struct called task_struct
    // each process has its own kernel stack
    // there's a structure called thread_info that contains stack pointers such as
    // rsp and esp that can be used to retrieve information about the active processes on the CPU
    // current process is defined in https://github.com/torvalds/linux/blob/master/include/linux/sched.h
    // real_cred is under line 934 with /* Process credentials: */

    // https://stackoverflow.com/questions/39229639/how-to-get-current-processs-uid-and-euid-in-linux-kernel-4-2/39230936
    const struct cred *cred = current_cred();

    printk(KERN_INFO "Current UID: %d\n", cred->euid.val);
    printk(KERN_INFO "OP: %d\n", op);

	/*if (cred->euid.val == 0) {
		printk(KERN_INFO "ROOT!\n");
		return 0;
	}*/

	/* 
	 * We allow everybody to read from our module, but
	 * only root (uid 0) may write to it 
	 * op 36 for read operation
	 * op 42 for write operation
	 */
	//if (op == 4 || (op == 2 && current->real_cred->euid == 0))
    if (op == 36 || (op == 42 && cred->euid.val == 0)) {
		printk(KERN_INFO "ACCESS ALLOWED!\n");
	// op 42???
    // cat op 36???
    //if (op == 34 || (op == 36 && cred->euid.val == 0))
		return 0;
	}

	printk(KERN_INFO "ACCESS DENIED!\n");

	/* 
	 * If it's anything else, access is denied 
	 */
	return -EACCES;
}

/* 
 * The file is opened - we don't really care about
 * that, but it does mean we need to increment the
 * module's reference count. 
 */
int procfs_open(struct inode *inode, struct file *file)
{
	try_module_get(THIS_MODULE); // see what this is!!
	return 0;
}

/* 
 * The file is closed - again, interesting only because
 * of the reference count. 
 */
int procfs_close(struct inode *inode, struct file *file)
{
	module_put(THIS_MODULE); // see what this is!!
	return 0;		/* success */
}

static struct file_operations file_ops = {
	.read 	 = procfs_read,
	.write 	 = procfs_write,
	.open 	 = procfs_open,
	.release = procfs_close,
};

/* 
 * Inode operations for our proc file. We need it so
 * we'll have some place to specify the file operations
 * structure we want to use, and the function we use for
 * permissions. It's also possible to specify functions
 * to be called for anything else which could be done to
 * an inode (although we don't bother, we just put
 * NULL). 
 */

static struct inode_operations inode_ops = {
	.permission = module_permission,	/* check for permissions */
};

/* 
 * Module initialization and cleanup 
 */
int init_module()
{
    // this is where /proc/buffer2k is created
	if(!(proc = proc_create(PROCFS_NAME, 0, NULL, &file_ops)))
                return -ENOMEM;

        // this works because we define the struct proc_dir_entry inside linux/internal.h
        proc->proc_iops = &inode_ops; // set the permissions function
        proc->mode = S_IFREG | S_IRUGO | S_IWUSR;

    printk(KERN_INFO "HELLOOOOOOOO\n");
	printk(KERN_INFO "/proc/%s created\n", PROCFS_NAME);

	return 0;	/* success */
}

void cleanup_module()
{
	//remove_proc_entry(PROCFS_NAME, proc_root);
    //remove_proc_entry(PROCFS_NAME, &proc_root);
	remove_proc_entry(PROCFS_NAME, NULL);
	printk(KERN_INFO "/proc/%s removed\n", PROCFS_NAME);
}

MODULE_LICENSE("GPL"); 