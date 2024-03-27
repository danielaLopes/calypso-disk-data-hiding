# Documentation
* followed https://wiki.tldp.org/lkmpg/en/content/ch08/2_6
* https://securityboulevard.com/2019/01/how-to-write-a-rootkit-without-really-trying/
* Glibc and System Calls: https://sys.readthedocs.io/en/latest/doc/07_calling_system_calls.html
* Control registers: https://en.wikipedia.org/wiki/Control_register#CR0
* Updated guide to finding the syscall table: https://infosecwriteups.com/linux-kernel-module-rootkit-syscall-table-hijacking-8f1bc0bd099c

# Locations
* /usr/include/sys/syscall.h
* /usr/include/bits/syscall.h

# Syscalls explained
* System call table stores an array of pointers, each pointer to a system call definition.
* When a process starts a system call, it places its number in a global register or on the stack and initiates a processor interrupt or trap.
* when a syscall happens, the process fills the registers with the appropriate values and then calls a special instruction which jumps to a previously defined location in the kernel (this location is readable by user processes, it is not writable by them), through interrupt 0x80 on Intel CPUs. The hardware knows that once you jump to this location, you are no longer running in restricted user mode, but as the OS kernel, and therefore you are allowed to do whatever you want.
* That location in the kernel that a process can jump to is called system_call. The procedure at that location checks the system call number, which tells the kernel what service the process requested. Then, it looks at the table of system calls (sys_call_table) to see the address of the kernel function to call. Then it calls the function, and after it returns, does a few system checks and then return back to the process (or to a different process, if the process time run out). This code is described in arch/<architecture>/kernel/entry.S, after the line ENTRY(system_call). For instance, it is described here: https://www.cs.dartmouth.edu/~sergey/cs258/rootkits/entry.S
* So, if we want to change the way a certain system call works, we need to write our own function to implement it, by adding our own code and then calling the original function, and then changing the pointer at sys_call_table to point to our function.

## Procedure to change sys_call_table
```
$ sudo cat /boot/System.map-5.4.0-42-generic | grep "sys_call_table"
```
* the returned address is the kernel location, so we cannot edit it because
it's write-protected. This is enabled by the CR0 control register, so we can overwrite it's value to toggle the write protection bit on pages marked as read only.
* The CR0 can be manipulated through two functions:
    * write_cr0() and read_cr0()
* We need to:
    1. read current bitmap of the CR0 register
    2. disable the write protection
    3. rewrite the CR0 register with new value

* With this we get the ability to edit the read-only portions of the kernel, so now we have to make the system call table writable, by marking the page where the system call table resides as writable.
* The kernel provides two functions to toggle write protection on pages: 
    * set_pages_ro() and set_pages_rw()

* With this we get the ability to edit the read-only portions of the kernel, so now we have to make the system call table writable, by marking the page where the system call table resides as writable.
* The kernel provides two functions to toggle write protection on pages: 
    * set_pages_ro() and set_pages_rw()

```
$ sudo cat /boot/System.map-5.4.0-42-generic | grep "pages_ro"
$ sudo cat /boot/System.map-5.4.0-42-generic | grep "pages_rw"
```

* These returned a virtual memory address, and we require the virtual page
* So we need to use virt_to_page() function defined in asm/page.h


## Why to change syscalls
* This process is called hooking, because we write a function for something that is called everytime something happens
* The most used hooks for rootkit are:
    * kill: mostly used to add some signals that can help communicate with your rootkit
    * getorentries: used to hide files and processes
    * read: used for keylogging
    * shutdown: to change shutdown sequence
    * ioctl: to change basic ioctl requests
    * ... and much more ...


## Why sys_call_table addresses change among different system files
* those are virtual addresses, but they all map to the same physical address