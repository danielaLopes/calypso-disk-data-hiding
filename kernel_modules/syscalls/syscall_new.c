/* 
 * Copyright (C) 2014 Drona Nagarajan 
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/unistd.h>
#include <asm/cacheflush.h>
#include <asm/page.h>

// $ sudo cat /boot/System.map-5.4.0-42-generic | grep "sys_call_table"
unsigned long *sys_call_table = (unsigned long *) 0xffffffff820013a0;

// sudo cat /boot/System.map-5.4.0-42-generic | grep "pages_ro"
void (*set_pages_ro) (struct page *page, int numpages) = (void *) 0xffffffff81085650;

// $ sudo cat /boot/System.map-5.4.0-42-generic | grep "pages_rw"
void (*set_pages_rw) (struct page *page, int numpages) = (void *) 0xffffffff810856c0;

asmlinkage int (*old_read)(unsigned int, const char __user *, size_t); //the original system to be hacked

/*
 * New read() syscall implementation
 */
int new_read(unsigned int fd, const char __user *buff, size_t buffsiz)
{
        printk(KERN_ALERT "READ HACKED!\n"); // do something more interesting, open a socket and connect to a remote server for instance.
        return old_read(fd, buff, buffsiz); // then call the original read system call.
}

/*
 * Replaces the appropriate location in sys_call_table and keeps the original
 * pointer in a variable
 * 
 * This replaces the read() syscall
 */
static int hijack_init(void)
{
        struct page *sys_call_page;
        sys_call_page = virt_to_page(&sys_call_table); // get the physical page address from the virtual memory address
        write_cr0(read_cr0() & ( ~ 0x10000)); // disable write protection... I guess you can figure out the bit arithmetic yourself :).
        set_pages_rw(sys_call_page, 1); // set the page as writable
        old_read = sys_call_table[__NR_read]; // backup the original system call
        sys_call_table[__NR_read] = new_read; // replace with your version
        set_pages_ro(sys_call_page, 1); // set the page back to read-only
        write_cr0(read_cr0() | (0x10000)); // enable write protection
        return 0;
}

/*
 * Uses the original sys_call_table pointer retrieved in hijack_init() to 
 * restore everything back to normal
 * 
 * This approach is dangerous because of the possibility of two kernel
 * modules changing the same system call.
 * Then, one of the modules will store the wrong original pointer and may
 * restore the pointer to that one instead of the correct one that the other
 * module has. This will cause the system to crash because it will no longer be in memory
 * 
 * We could check if the system call is equal to our open function and if so,
 * do nothing, but this will cause an even greater problem because the last 
 * module to be removed may have still stored the wrong pointer and will still
 * replace the sys_call_table pointer with that one, so the system will still crash
 */
void hijack_stop(void)
{
        struct page *sys_call_page;
        sys_call_page = virt_to_page(&sys_call_table);
        write_cr0(read_cr0() & ( ~ 0x10000));
        set_pages_rw(sys_call_page, 1); // set the page as writable
        sys_call_table[__NR_read] = old_read; // rewrite original system call.
        set_pages_ro(sys_call_page, 1);
        write_cr0(read_cr0() | (0x10000));
}

module_init(hijack_init);
module_exit(hijack_stop);