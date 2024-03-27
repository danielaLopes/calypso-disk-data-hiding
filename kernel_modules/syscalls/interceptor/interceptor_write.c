#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/delay.h>
#include <asm/paravirt.h>

/*
    A system call table is a set of pointer to functions that implement system calls
    To see the list of system calls implemented in the kernel, see /usr/include/bits/syscall.h
*/

unsigned long **sys_call_table;
unsigned long original_cr0;

asmlinkage long (*ref_sys_write)(unsigned int fd, char __user *buf, size_t count);
asmlinkage long new_sys_write(unsigned int fd, char __user *buf, size_t count)
{
	long ret;
	ret = ref_sys_write(fd, buf, count);

    printk(KERN_INFO "new_sys_write()");

	if(count == 1 && fd == 0)
		printk(KERN_INFO "intercept write at: 0x%02X", buf[0]);

	return ret;
}

static unsigned long **aquire_sys_call_table(void)
{
	unsigned long int offset = PAGE_OFFSET;
	unsigned long **sct;

    printk(KERN_INFO "aquire_sys_call_table()");

	while (offset < ULLONG_MAX) {
		sct = (unsigned long **)offset;

		if (sct[__NR_close] == (unsigned long *) ksys_close) 
			return sct;

		offset += sizeof(void *);
	}
	
	return NULL;
}

static int __init interceptor_start(void) 
{
    printk(KERN_INFO "write interceptor_start()");

	if(!(sys_call_table = aquire_sys_call_table()))
		return -1;
	
    printk(KERN_INFO "acquired sys_call_table at %p", &sys_call_table);

    /*
        cr0 is one of the control registers
        read_cr0() reads the value of the register
        write_cr0() writes to this register

        (original_cr0 & ~0x00010000) is a mask to set bit 16, the write protect bit
        So it removes the write protection on the page with the syscall table
    */
	original_cr0 = read_cr0();

	write_cr0(original_cr0 & ~0x00010000); // changes the write protection bit that allowing writing to the syscall table
	ref_sys_write = (void *)sys_call_table[__NR_write];
	sys_call_table[__NR_read] = (unsigned long *)new_sys_write;
	write_cr0(original_cr0);
	
	return 0;
}

static void __exit interceptor_end(void) 
{
    printk(KERN_INFO "write interceptor_end()");

	if(!sys_call_table) {
		return;
	}
	
	write_cr0(original_cr0 & ~0x00010000);
	sys_call_table[__NR_write] = (unsigned long *)ref_sys_write;
	write_cr0(original_cr0);
	
	msleep(2000);
}

module_init(interceptor_start);
module_exit(interceptor_end);

MODULE_LICENSE("GPL");