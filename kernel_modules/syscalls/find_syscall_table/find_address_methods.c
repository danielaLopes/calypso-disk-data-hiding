/* 
 * 
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/unistd.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>      /* set_fs() */

#define unprotect_memory(orig_cr0) \
({ \
	orig_cr0 =  read_cr0();\
	write_cr0(orig_cr0 & (~ 0x10000)); /* Set WP flag to 0 */ \
});
#define protect_memory(orig_cr0) \
({ \
	write_cr0(orig_cr0); /* Set WP flag to 1 */ \
});

int orig_cr0;  //Used to store the original value of cr0 register
asmlinkage long (*orig_read)(int, int);
unsigned long *sys_call_table;

void hooking_syscall(void *hook_addr, uint16_t syscall_offset, unsigned long *sys_call_table)
{
    printk(KERN_INFO "hooking_syscall");
	//unprotect_memory(orig_cr0);
    orig_cr0 =  read_cr0();
    printk(KERN_INFO "after orig_cr0\n");
    printk(KERN_INFO "%d\n", orig_cr0);
    write_cr0(read_cr0() | 0x10000);
	//write_cr0(orig_cr0 & (~ 0x10000));
	printk(KERN_INFO "after write_cr0\n");
    sys_call_table[syscall_offset] = (unsigned long)hook_addr;
    printk(KERN_INFO "after sys_call_table[syscall_offset]\n");
	//protect_memory(orig_cr0);
    //write_cr0(orig_cr0);
}

void unhooking_syscall(void *orig_addr, uint16_t syscall_offset, void *hook_addr)
{
    printk(KERN_INFO "unhooking_syscall");
	// unprotect_memory(orig_cr0);
	// sys_call_table[syscall_offset] = (unsigned long)hook_addr;
	// protect_memory(orig_cr0);
}

asmlinkage int hooked_read(int magic1, int magic2)
{
	printk(KERN_INFO "Hello from hook!");
	return orig_read(magic1, magic2);
}

/*
 * ________________________________________________________________
 * 
 * Initializations for /proc/kallsyms method
 * ________________________________________________________________
 */

/*
 * Enable kernel address space which is 4G
*/
// #define ENTER_KERNEL_ADDR_SPACE(oldfs) \
// ({ \
// 	oldfs = get_fs();  \
// 	set_fs (KERNEL_DS); \
// });

/*
 * Enable user address space which is 3G
 */
// #define EXIT_KERNEL_ADDR_SPACE(oldfs) \
// ({ \
// 	set_fs(oldfs); \
// });


/*
 * ________________________________________________________________
 * 
 * Implementation of loop memory method
 * ________________________________________________________________
 */

/*
 * run over the memory till find the sys call table
 * doing so, by searching the sys call close.
 * 
 * We need to start on a memory address we know for sure is before
 * the sys_call_table address in memory. Then, we loop through the
 * memory, until we find a place that when it's added with a certain
 * offset, it will point to an address we searched with. We chose sys_close
 * 
 * Note: ksys_close() is no longer exposed since kernel 4
 */



/*
 * ________________________________________________________________
 * 
 * Implementation of /proc/kallsyms method
 * ________________________________________________________________
 */

/*
 * /proc/kallsyms file contains all the symbols of the dynamically loaded
 * kernel modules and the static code's symbols, which is equivalent to 
 * having the whole kernel mapping in one place
 * 
 * So we just need to read the file and search for the sys_call_table_symbol
 * 
 * $ sudo cat /proc/kallsyms | grep "sys_call_table"
 * > ffffffffbb800280 R x32_sys_call_table
 * > ffffffffbb8013a0 R sys_call_table
 * > ffffffffbb8023e0 R ia32_sys_call_table
 * 
 * However, due to the protection rings, if we are in the kernel space, we can only
 * read from the kernel space, and the same for user space.
 * In order to read user space from a kernel module, we can change the global variable
 * addr_limit, which is the highest address unprivileged code is allowed to access.
 * To do that, we use set_fs(): https://lwn.net/Articles/722267/
 */
/* 
 * Retirve the address of syscall table from 
 * for kernel version >= 2.6 using file `/proc/kallsmys`
 * for kernel version < 2.6 using file `/proc/ksyms`
 */
// unsigned long * obtain_syscall_table_by_proc(void)
// {
// 	char *file_name                       = PROC_KSYMS;
// 	int i                                 = 0;         /* Read Index */
// 	struct file *proc_ksyms               = NULL;      /* struct file the '/proc/kallsyms' or '/proc/ksyms' */
// 	char *sct_addr_str                    = NULL;      /* buffer for save sct addr as str */
// 	char proc_ksyms_entry[MAX_LEN_ENTRY]  = {0};       /* buffer for each line at file */
// 	unsigned long* res                    = NULL;      /* return value */ 
// 	char *proc_ksyms_entry_ptr            = NULL;
// 	int read                              = 0;
// 	mm_segment_t oldfs;


// 	/* Allocate place for sct addr as str */
// 	if((sct_addr_str = (char*)kmalloc(MAX_LEN_ENTRY * sizeof(char), GFP_KERNEL)) == NULL)
// 		goto CLEAN_UP;
	
// 	if(((proc_ksyms = filp_open(file_name, O_RDONLY, 0)) || proc_ksyms) == NULL)
// 		goto CLEAN_UP;

//     /*
//      * 1. Use set_fs() to set the addr_limit so we can read /proc/kallsyms
//      * file from the user space.
//      */
// 	ENTER_KERNEL_ADDR_SPACE(oldfs);

//     /*
//      * 2. we read it using vfs_read()
//      */
// 	read = vfs_read(proc_ksyms, proc_ksyms_entry + i, 1, &(proc_ksyms->f_pos));
	
//     /*
//      * 3. we return addr_limit to the original value, which is very important because
//      * if we don't do it, any user mode process could manipulate the kernel address space
//      */
//     EXIT_KERNEL_ADDR_SPACE(oldfs);
	
//     /*
//      * 4. in every iteration, we check to see if the line contains "sys_call_table" string,
//      * and if so, save the address and return it
//      */
// 	while( read == 1)
// 	{
// 		if(proc_ksyms_entry[i] == '\n' || i == MAX_LEN_ENTRY)
// 		{
// 			if(strstr(proc_ksyms_entry, "sys_call_table") != NULL)
// 			{
// 				printk(KERN_INFO "Found Syscall table\n");
// 				printk(KERN_INFO "Line is:%s\n", proc_ksyms_entry);

// 				proc_ksyms_entry_ptr = proc_ksyms_entry;
// 				strncpy(sct_addr_str, strsep(&proc_ksyms_entry_ptr, " "), MAX_LEN_ENTRY);
// 				if((res = kmalloc(sizeof(unsigned long), GFP_KERNEL)) == NULL)
// 					goto CLEAN_UP;
// 				kstrtoul(sct_addr_str, 16, res);
// 				goto CLEAN_UP;
// 			}

// 			i = -1;
// 			memset(proc_ksyms_entry, 0, MAX_LEN_ENTRY);
// 		}
	
// 		i++;
    
// #if LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0)
// 	read = kernel_read(proc_ksyms, proc_ksyms_entry + i, 1, &(proc_ksyms->f_pos));
// #else
// 	ENTER_KERNEL_ADDR_SPACE();
// 	read = vfs_read(proc_ksyms, proc_ksyms_entry + i, 1, &(proc_ksyms->f_pos));
// 	EXIT_KERNEL_ADDR_SPACE();
// #endif

// 	}


// CLEAN_UP:
// 	if(sct_addr_str != NULL)
// 		kfree(sct_addr_str);
// 	if(proc_ksyms != NULL)
// 		filp_close(proc_ksyms, 0);

// 	return (unsigned long*)res;
// }


/*
 * ________________________________________________________________
 * 
 * Implementation of kallsyms_lookup_name() method
 * ________________________________________________________________
 */
unsigned long *obtain_syscall_table_kalsyms_lookup(void)
{
    return kallsyms_lookup_name("sys_call_table");
}


/*
 * ________________________________________________________________
 * 
 * Init and exit module functions
 * ________________________________________________________________
 */

/*
 * 
 */
static int hijack_init(void)
{       
    sys_call_table = obtain_syscall_table_kalsyms_lookup();
    printk(KERN_INFO "Address through kalsyms lookup1: %p\n", (void *)sys_call_table);
    
    printk(KERN_INFO "BEFORE ORIG_READ");
    printk(KERN_INFO "BEFORE ORIG_READ %d\n", __NR_read );
	orig_read = (void*)sys_call_table[__NR_read];
    printk(KERN_INFO "AFTER ORIG_READ\n");

	hooking_syscall(hooked_read, __NR_read, sys_call_table);
    printk(KERN_INFO "AFTER hooking syscall\n");

    return 0;
}

/*
 * 
 */
void hijack_stop(void)
{
    printk(KERN_INFO "Stopped finding the sys_call_table with several methods");
    //unhooking_syscall(orig_read, __NR_read, sys_call_table);
}

MODULE_LICENSE("GPL");

module_init(hijack_init);
module_exit(hijack_stop);