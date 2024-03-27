#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <asm/cacheflush.h>
#include <asm/page.h>
#include <asm/current.h>
#include <linux/sched.h>
#include <linux/kallsyms.h>
 
unsigned long *syscall_table; 
 
asmlinkage int (*original_write)(unsigned int, const char __user *, size_t);
 
asmlinkage int new_write(unsigned int fd, const char __user *buf, size_t count) {
 
    // hijacked write
 
    printk(KERN_ALERT "WRITE HIJACKED");
 
    return (*original_write)(fd, buf, count);
}
 
static int hijack_init(void) {
 
    printk(KERN_INFO "HIJACK INIT\n");

    syscall_table = (void*) kallsyms_lookup_name("sys_call_table");
 
    write_cr0 (read_cr0 () & (~ 0x10000));
 
    original_write = (void *)syscall_table[__NR_write];
    syscall_table[__NR_write] = new_write;  
 
    write_cr0 (read_cr0 () | 0x10000);
 
    return 0;
}
 
static void hijack_exit(void) {
 
    write_cr0 (read_cr0 () & (~ 0x10000));
 
    syscall_table[__NR_write] = original_write;  
 
    write_cr0 (read_cr0 () | 0x10000);
 
    printk(KERN_INFO "MODULE EXIT\n");
 
    return;
}
 
MODULE_LICENSE("GPL");

module_init(hijack_init);
module_exit(hijack_exit);