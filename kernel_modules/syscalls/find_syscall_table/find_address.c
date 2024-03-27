#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/unistd.h>
#include <linux/kallsyms.h>
 
//unsigned long *syscall_table = (unsigned long *)0xTABLE; 
unsigned long *syscall_table;

static int find_address_init(void) {
 
    printk( KERN_INFO "\nFINDING SYSCALL TABLE ADDRESS\n");
 
    //syscall_table[__NR_write] = (unsigned long *)kallsyms_lookup_name("sys_call_table"
    syscall_table = (unsigned long *)kallsyms_lookup_name("sys_call_table");
    
    printk( KERN_INFO "\nADDRESS IS: %lu\n", &syscall_table);

    return 0;
}
 
static void find_address_exit(void) {
 
    printk( KERN_INFO "FOUND IT!\n");
}
 
module_init(find_address_init);
module_exit(find_address_exit);

MODULE_LICENSE("GPL");