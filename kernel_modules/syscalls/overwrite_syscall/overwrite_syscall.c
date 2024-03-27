#include <linux/init.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>

unsigned long *syscall_table = NULL;
// corresponds to the syscall that will be replaced
asmlinkage long (*original)(unsigned int size, unsigned long *ptr);
unsigned long original_cr0;

#define SYSCALL_TO_REPLACE _NR_epoll_ctl_old

/*
In alternative, replace read or write with this:

asmlinkage int new_syscall(unsigned int fd, const char _user *buf, size_t count)
{
    printk("who is calling the big bad write ?\n"); 
    return (*original)(fd, buf, count);
}
*/ 
asmlinkage int new_syscall(unsigned int size, unsigned long *ptr)
{
  printk("I'm the syscall which overwrittes the previous !\n");
  return 0;
}

static unsigned long **find_syscall_table(void)
{
  unsigned long int offset = PAGE_OFFSET;
  unsigned long **sct;

  while (offset < ULLONG_MAX) {
    sct = (unsigned long **)offset;

    if (sct[_NR_close] == (unsigned long *) sys_close)
      return sct;

    offset += sizeof(void *);
  }

  return NULL;
}

static int _init overwrite_init(void)
{
  syscall_table = (void **) find_syscall_table();
  if (syscall_table == NULL) {
    printk(KERN_ERR"net_malloc: Syscall table is not found\n");
    return -1;
  }
  // wrapper for asm part
  original_cr0 = read_cr0();
  write_cr0(original_cr0 & ~0x00010000);
  original = (void *)syscall_table[SYSCALL_TO_REPLACE];

  // we now overwrite the syscall
  syscall_table[SYSCALL_TO_REPLACE] = (unsigned long *)new_syscall;
  write_cr0(original_cr0);
  printk("net_malloc: Patched! syscall number : %d\n", SYSCALL_TO_REPLACE);
  return 0;
}

static void _exit overwrite_exit(void)
{
  // reset overwritten syscall
  if (syscall_table != NULL) {
    original_cr0 = read_cr0();
    write_cr0(original_cr0 & ~0x00010000);

    syscall_table[SYSCALL_TO_REPLACE] = (unsigned long *)original;

    write_cr0(original_cr0);
  }
}

module_init(overwrite_init);
module_exit(oveerwrite_exit);