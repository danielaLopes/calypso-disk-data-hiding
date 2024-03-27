/* 
 * Copyright (C) 2001 by Peter Jay Salzman 
 */

#include <linux/kernel.h>       /* We're doing kernel work */
#include <linux/module.h>       /* Specifically, a module, */
#include <linux/moduleparam.h>  /* which will have params */
#include <linux/unistd.h>       /* The list of system calls */
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <linux/cred.h>	        /* to call current_cred() */

/* this used to work on 2.4 kernels, but now the system call table
is no longer exportable */
/* This will not compile: ERROR: "sys_call_table" undefined! */
extern void *sys_call_table[];

/* we can also do the following on the command line */
// $ sudo cat /boot/System.map-5.4.0-42-generic | grep "sys_call_table"
// > ffffffff82000280 R x32_sys_call_table
// > ffffffff820013a0 R sys_call_table <- this one!
// > ffffffff820023e0 R ia32_sys_call_table

/* this address is the kernel location, so we cannot edit it because
 * it's write-protected. This is enabled by the CR0 control register, so
 * we can overwrite it's value to toggle the write protection bit on pages 
 * marked as read only.
 * The CR0 can be manipulated through two functions:
 * write_cr0() and read_cr0()
 * We need to:
 *  1. read current bitmap of the CR0 register
 *  2. disable the write protection
 *  3. rewrite the CR0 register with new value
 * 
 * Control registers reference: https://en.wikipedia.org/wiki/Control_register#CR0
 * 
 * With this we get the ability to edit the read-only portions of the kernel,
 * so now we have to make the system call table writable, by marking the page where
 * the system call table resides as writable.
 * The kernel provides two functions to toggle write protection on pages: 
 * set_pages_ro() and set_pages_rw()
 * 
 * $ sudo cat /boot/System.map-5.4.0-42-generic | grep "pages_ro"
 * > ffffffff81085650 T set_pages_ro
 * 
 * $ sudo cat /boot/System.map-5.4.0-42-generic | grep "pages_rw"
 * > ffffffff810856c0 T set_pages_rw
 * 
 * These returned a virtual memory address, and we require the virtual page
 * So we need to use virt_to_page() function defined in asm/page.h
 */

static int uid;
module_param(uid, int, 0644);
asmlinkage int (*original_call) (const char *, int, int); // pointer to the original system call

asmlinkage int our_sys_open(const char *filename, int flags, int mode)
{
  int i = 0;
  char ch;

  const struct cred *cred = current_cred();

  /* 
   * Check if this is the user we're spying on 
   */
  if (uid == cred->euid.val) {
    /* 
     * Report the file, if relevant 
     */
    printk("Opened file by %d: ", uid);
    do {
      get_user(ch, filename + i);
      i++;
      printk("%c", ch);
    } while (ch != 0);
    printk("\n");
  }
  return original_call(filename, flags, mode);
}

/* 
 * Initialize the module - replace the system call 
 */
int init_module()
{
  printk(KERN_ALERT "I'm dangerous. I hope you did a ");
  printk(KERN_ALERT "sync before you insmod'ed me.\n");
  printk(KERN_ALERT "My counterpart, cleanup_module(), is even");
  printk(KERN_ALERT "more dangerous. If\n");
  printk(KERN_ALERT "you value your file system, it will ");
  printk(KERN_ALERT "be \"sync; rmmod\" \n");
  printk(KERN_ALERT "when you remove this module.\n");

  /* 
   * Keep a pointer to the original function in
   * original_call, and then replace the system call
   * in the system call table with our_sys_open 
   */
  original_call = sys_call_table[__NR_open];
  sys_call_table[__NR_open] = our_sys_open;

  printk(KERN_INFO "Spying on UID:%d\n", uid);

  return 0;
}

/* 
 * Cleanup - unregister the appropriate file from /proc 
 */
void cleanup_module()
{
  /* 
   * Return the system call back to normal 
   */
  if (sys_call_table[__NR_open] != our_sys_open) {
    printk(KERN_ALERT "Somebody else also played with the ");
    printk(KERN_ALERT "open system call\n");
    printk(KERN_ALERT "The system may be left in ");
    printk(KERN_ALERT "an unstable state.\n");
  }

  sys_call_table[__NR_open] = original_call;
}