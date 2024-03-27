# Documentation
* https://sysplay.github.io/books/LinuxDrivers/book/Content/Part01.html
* New block device documentation: https://prog.world/linux-kernel-5-0-we-write-simple-block-device-under-blk-mq/

# Device driver
* manages and controls devices
* provides a system call interface to the user

# Device
* any peripheral connected to a a computer, harddisk, ...

# Loadable kernel modules
* the kernel is an object oriented implementation in C. Every driver consists of a constructor (in insmod) and a destructor (in rmmod).

# Kernel logging
```
#define KERN_EMERG    "<0>" /* system is unusable            */
#define KERN_ALERT    "<1>" /* action must be taken immediately    */
#define KERN_CRIT    "<2>" /* critical conditions            */
#define KERN_ERR    "<3>" /* error conditions            */
#define KERN_WARNING    "<4>" /* warning conditions            */
#define KERN_NOTICE    "<5>" /* normal but significant condition    */
#define KERN_INFO    "<6>" /* informational                */
#define KERN_DEBUG    "<7>" /* debug-level messages            */
``` 

# Error handling
* return a negative number for an error, generally a minus sign appended with a macro from linux/errno.h

* return 0 for success

* return positive number to indicate information like number of bytes transferred


# Hardware access
## List memory map of the system
* shows the I/O memory regions registered on the system
```
sudo cat /proc/iomem
```

* Output: 
```
00000000-00000fff : Reserved        // Reserved = not used, not available for anything
00001000-0009fbff : System RAM      // Regular memory - can be used to store code or data
0009fc00-0009ffff : Reserved
000a0000-000bffff : PCI Bus 0000:00 // PCI device memory case
000c0000-000c7fff : Video ROM       // VGA BIOS memory
000e2000-000e2fff : Adapter ROM     // General area for "boot ROM", e.g. Network boot
000d2000-000d3fff : reserved    
000f0000-000fffff : Reserved
000f0000-000fffff : System ROM      // System BIOS
00100000-dffeffff : System RAM      // Regular RAM
bac00000-bba00eb0 : Kernel code
bba00eb1-bc45763f : Kernel data
bc720000-bcbfffff : Kernel bss      // Kernel "initialized to zero" data.
dfff0000-dfffffff : ACPI Tables     
e0000000-fdffffff : PCI Bus 0000:00
e0000000-e0ffffff : 0000:00:02.0
e0000000-e0ffffff : vmwgfx probe
f0000000-f01fffff : 0000:00:02.0
f0000000-f01fffff : vmwgfx probe
f0200000-f021ffff : 0000:00:03.0
f0200000-f021ffff : e1000
f0400000-f07fffff : 0000:00:04.0
f0400000-f07fffff : vboxguest
f0800000-f0803fff : 0000:00:04.0
f0804000-f0804fff : 0000:00:06.0
f0804000-f0804fff : ohci_hcd
f0805000-f0805fff : 0000:00:0b.0
f0805000-f0805fff : ehci_hcd
f0806000-f0807fff : 0000:00:0d.0
f0806000-f0807fff : ahci
fec00000-fec00fff : Reserved
fec00000-fec003ff : IOAPIC 0
fee00000-fee00fff : Local APIC
fee00000-fee00fff : Reserved
fffc0000-ffffffff : Reserved
100000000-11fffffff : System RAM
```

## Obtain approximate RAM size on the system
```
sudo cat /proc/meminfo
```

```
MemTotal:        4030880 kB
MemFree:         1721804 kB
MemAvailable:    3051636 kB
Buffers:           65660 kB
Cached:          1423300 kB
SwapCached:            0 kB
Active:          1269496 kB
Inactive:         840556 kB
Active(anon):     608640 kB
Inactive(anon):     7356 kB
Active(file):     660856 kB
Inactive(file):   833200 kB
Unevictable:          32 kB
Mlocked:              32 kB
SwapTotal:       1961760 kB
SwapFree:        1961760 kB
Dirty:               112 kB
Writeback:             0 kB
AnonPages:        621124 kB
Mapped:           322372 kB
Shmem:              8280 kB
KReclaimable:      77632 kB
Slab:             141168 kB
SReclaimable:      77632 kB
SUnreclaim:        63536 kB
KernelStack:        6928 kB
PageTables:        11620 kB
NFS_Unstable:          0 kB
Bounce:                0 kB
WritebackTmp:          0 kB
CommitLimit:     3977200 kB
Committed_AS:    3823352 kB
VmallocTotal:   34359738367 kB
VmallocUsed:       33748 kB
VmallocChunk:          0 kB
Percpu:              732 kB
HardwareCorrupted:     0 kB
AnonHugePages:         0 kB
ShmemHugePages:        0 kB
ShmemPmdMapped:        0 kB
FileHugePages:         0 kB
FilePmdMapped:         0 kB
CmaTotal:              0 kB
CmaFree:               0 kB
HugePages_Total:       0
HugePages_Free:        0
HugePages_Rsvd:        0
HugePages_Surp:        0
Hugepagesize:       2048 kB
Hugetlb:               0 kB
DirectMap4k:      171968 kB
DirectMap2M:     4022272 kB
```

## Accessing physical (RAM) addresses
* https://lwn.net/Articles/102232/

* In Linux, these are not directly accessible since they are mapped to virtual addresses. These mappings can be made through the following API for mapping and unmapping addresses:
```
#include <asm/io.h>

void *ioremap(unsigned long device_bus_address, unsigned long device_region_size);
void iounmap(void *virt_addr);
```

* ioremap() returns a cookie which can be passes to read and write functions to move data from and to I/O memory.

* to know to which set of device registers or memory to use, by adding their offsets to the virtual address returned by ioremap()
```
unsigned int ioread8(void *virt_addr);
unsigned int ioread16(void *virt_addr);
unsigned int ioread32(void *virt_addr);
unsigned int iowrite8(u8 value, void *virt_addr);
unsigned int iowrite16(u16 value, void *virt_addr);
unsigned int iowrite32(u32 value, void *virt_addr);
```

## ioctl()
* if there is no system call that meets the requirement, ioctl() is the one to use
* can be used for instance, to debug a driver by querying its data structures
* this is achieved by a single function prototype with the command and command's argument. The command is just some number that represents the operation, and the argument is the paramenter for the function.
* the ioctl() function should implement a switch case.

* signature for function that should be implemented in kernel space: 
    * long ioctl(struct file *f, unsigned int cmd, unsigned long arg);
    * both the command and argument type  definitions need to be shared across the driver (in kernel space) and the application (in user space), so they are commonly placed in header files.

* if more than an argument is required, all of the arguments should be put in a structure and the arg parameter should be the pointer to that structure (unsigned long fits all)

* signature for ioctl function to be called by the user:
    * int ioctl(int fd, int cmd, ...);

* see asm-generic/ioctl.h


## Debugging
* we need the complete kernel installed, not just the kernel headers. To load and build modules, the headers are sufficient

# kgdb tutorial
* minimal debugging server


## Hard disk
* partition table has 4 entries max
* however, extended partitions can have more partitions (logical partitions), which allows an unlimited number of partitions
* metadata for logical partitions is maintained in a linked list

* MBR (main boot record): stores the linked list head of the partition table for the logical partitions
* LBR (logical boot record): stores subsequent linked list nodes