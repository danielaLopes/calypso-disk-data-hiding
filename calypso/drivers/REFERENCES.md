# General
* General resources for OS development: https://stackoverflow.com/questions/43180/what-are-some-resources-for-getting-started-in-operating-system-development

* kernel documentation: https://www.kernel.org/doc/html/latest//search.html

* Linux kernel reference: https://www.halolinux.us/kernel-reference/

* Linux API: https://www.cs.bham.ac.uk/~exr/teaching/lectures/opsys/13_14/docs/kernelAPI/

## Youtube tutorials
* Kernel Hacking Tutorial series: https://www.youtube.com/watch?v=8dZFJEc-8uI&ab_channel=SourceCodeDeleted


# Makefile and compilling
* place out of tree kernel modules (external modules) in /lib/modules/, which are parsed by depmod

* create out of tree kernel modules and Makefile: https://www.kernel.org/doc/Documentation/kbuild/modules.txt

# Block device driver guides
* https://olegkutkov.me/2020/02/10/linux-block-device-driver/

* Both RAM disk and writing data to the disk:
    * https://linux-kernel-labs.github.io/refs/heads/master/labs/block_device_drivers.html#block-device
    * https://github.com/linux-kernel-labs/linux


# I/O operations
## Read and write to specific sectors of the disk (block layer)
* has code for reading and writing to pages using bio: https://stackoverflow.com/questions/12720420/how-to-read-a-sector-using-a-bio-request-in-linux-kernel

* https://sysplay.in/blog/tag/sb_bread/

## Read and write to a file (filesystem layer)
* Semester project: make a file system: https://sysplay.github.io/books/LinuxDrivers/book/Content/Part18.html

* Read section 7.6 a baby file system: https://www.win.tue.nl/~aeb/linux/lk/lk-7.html

## Transfering blocks between bio, memory and disk sectors
* explains what happens with memory pages on reads and writes and how they are related to block devices and block size: https://www.halolinux.us/kernel-reference/sectors-blocks-and-buffers.html

* Relates file reading with the block layer: https://stackoverflow.com/questions/29762313/how-to-read-a-sector-using-bio-request-in-linux-kernel

* Detailed review of handling I/O requests in the block layer: https://www.oreilly.com/library/view/linux-device-drivers/0596000081/ch12s04.html

## block sizes and limitations
* https://stackoverflow.com/questions/30585565/bypassing-4kb-block-size-limitation-on-block-layer-device

* All disk units explained and relation to memory: https://sites.google.com/site/knsathyawiki/example-page/chapter-14-the-block-i-o-layer


## Writing and reading blocks to and from files
### How to read and write to files from the kernel 
* (up to date): https://stackoverflow.com/questions/1184274/read-write-files-within-a-linux-kernel-module

* (is not up to date): https://www.linuxjournal.com/article/8110

* Reading files into Linux Kernel Memory (is not up to date): http://www.ouah.org/mammon_kernel-mode_read_hack.txt

* example of source code where files are read and written from kernel mode (__kernel_write()): https://elixir.bootlin.com/linux/latest/source/kernel/acct.c

#### kernel functions to read and write file
* fwrite() vs write() vs pwrite() vs fread() vs read() vs pread() vs fsync(): https://stackoverflow.com/questions/41373309/whats-the-difference-between-fwrite-write-pwrite-fread-read-prea

### Random access to file
* why append mode always appends sequentially to the end of the file: https://www.gnu.org/software/libc/manual/html_node/File-Position.html

* random access to file in kernel space: https://stackoverflow.com/questions/56721527/linux-vfs-vfs-read-offset-vs-vfs-llseek-pos-how-to-seek-in-a-fil

### Write() and read() system calls to a file
* How write() and read() system calls are implemented. Reading a file is page based. If a process issues a read() system call to get a few bytes and that data is not already in RAM, the kernel allocates a new page frame, fills the page with the suitable portion of the file, adds the page to the page cache, and finally copies the requested bytes into the process address space: https://www.oreilly.com/library/view/understanding-the-linux/0596005652/ch16s01.html

### file->f_op->read and file->f_op->read_iter
* https://www.win.tue.nl/~aeb/linux/vfs/trail-3.html

* https://stackoverflow.com/questions/33183923/unable-to-open-read-text-file-from-kernel-module-on-linux-kernel-version-4-2-3

* https://stackoverflow.com/questions/57730621/cant-find-where-vfs-read-function-calls-ext4-file-read-iter-function-in-l


## from buffer_head to bio:
* Explain transition from kernel 2.5 to 2.6 and reason to add bio to process requests instead of buffer_head: https://www.kernel.org/doc/Documentation/block/biodoc.txt



# Kernel Datastructures
## Block device drivers general schemes
* Request example with memory interactions: https://bootlin.com/doc/legacy/block-drivers/block_drivers.pdf

* Most important data structures defined: https://www.programmersought.com/article/44482750066/




## struct bio
* explain how to use it, API and brief scheme: https://lwn.net/Articles/26404/

* how it works, relation with segments, sectors, memory and request queue: https://stackoverflow.com/questions/31951233/linux-kernel-struct-bio-how-pages-are-read-written



# User space vs kernel space
* Linux API (see 4.2 User space memory access): https://www.cs.bham.ac.uk/~exr/teaching/lectures/opsys/13_14/docs/kernelAPI/

* Explain user memory kernel API: https://developer.ibm.com/technologies/linux/articles/l-kernel-memory-access/

## Access user mode from kernel mode and SMAP (supervisor mode access prevention)
* can kernel access user mode?: https://stackoverflow.com/questions/15251527/kernel-mode-can-it-access-to-user-mode

## How to check if memory address can be accessed from the kernel
* https://stackoverflow.com/questions/29251145/check-whether-memory-address-can-be-accessed-from-linux-kernel-space


# Virtual block device that acts as the front-end of another block device
* stackdb: https://orenkishon.wordpress.com/2014/10/29/stackbd-stacking-a-block-device-over-another-block-device/
    * code: https://github.com/OrenKishon/stackbd
    * Acessing block device from kernel module; "You don't need to access the other device's gendisk structure, but rather its request queue. You can prepare I/O requests and push it down the other device's queue, and it will do the rest itself.": https://stackoverflow.com/questions/4425816/accesing-block-device-from-kernel-module

* O_EXCL device open mode explained and bio vs request layers: https://lwn.net/Articles/736534/

* Reading from block device in kernel space: https://stackoverflow.com/questions/1695678/reading-from-a-block-device-in-kernel-space

## Modifying requests (bio) to redirect to another devices
* https://stackoverflow.com/questions/48054703/create-a-non-trivial-device-mapper-target

* 

## Outdated block layer IO hooking
* (slightly more updated make_request_fn) filter driver hook: https://www.msystechnologies.com/blog/understanding-device-mapper-and-filter-driver/

* Snoop on another device's requests (by overwritting the request handling function of the queue, out of date): https://github.com/asimkadav/block-filter/blob/master/misc/misc.c

* https://github.com/veeam/veeamsnap

* https://github.com/datto/dattobd

* https://gist.github.com/prashants/3839380

## Updated block layer IO hooking
* Interception used to be based on replacing make_request_fn function, which is not possible anymore. Also mentions why Device Mapper is not the solution. This implements an API that makes interception possible again: https://github.com/CodeImp/blk-filter


# Device Mapper
* Writing a Device Mapper Target and Device Mapper Tables: https://gauravmmh1.medium.com/writing-your-own-device-mapper-target-539689d19a89

* Very good schemes: https://xuechendi.github.io/2013/11/14/device-mapper-deep-dive

* code for target: https://stackoverflow.com/questions/24214289/create-device-mapper-target

# Kernel flow and layers
* The Linux storage stack scheme: https://en.wikipedia.org/wiki/Virtual_file_system#/media/File:The_Linux_Storage_Stack_Diagram.svg

## Device mappers, block devices and requests
* https://xuechendi.github.io/2013/11/14/device-mapper-deep-dive



# Debugging the kernel
* https://blogs.oracle.com/linux/extracting-kernel-stack-function-arguments-from-linux-x86-64-kernel-crash-dumps

* https://www.dedoimedo.com/computers/crash-analyze.html

## Kernel Oops: 0002
* https://ask.csdn.net/questions/3162411

## addr2line
* https://elinux.org/Addr2line_for_kernel_debugging

* https://stackoverflow.com/questions/50242559/addr2line-on-kernel-address-cannot-find-source-line

## Linux registers
* https://stackoverflow.com/questions/36529449/why-are-rbp-and-rsp-called-general-purpose-registers

## Kernel tracing
* https://www.kernel.org/doc/html/v5.4/trace/events.html

## kprobes
* https://events19.linuxfoundation.org/wp-content/uploads/2017/12/oss-eu-2018-fun-with-dynamic-trace-events_steven-rostedt.pdf


# File systems
* kernel documentation fs overview: https://www.kernel.org/doc/html/latest/filesystems/vfs.html
    * https://dri.freedesktop.org/docs/drm/filesystems/index.html

* super_block fields explained: https://www.halolinux.us/kernel-reference/superblock-objects.html

* really cool scheme about file systems data structures relations and VFS vs filesystem: https://github.com/rgouicem/ouichefs

* VFS structures and operations explained: https://devarea.com/wp-content/uploads/2017/10/Linux-VFS-and-Block.pdf

* VFS operations: https://tldp.org/LDP/khg/HyperNews/get/fs/vfstour.html

## Sample toy filesystems
* https://github.com/jserv/simplefs
* https://github.com/gishsteven/Toy-VFS

## Understanding how operations work
* rm under the hood: http://events17.linuxfoundation.org/sites/events/files/slides/Unrm_%20Hacking%20the%20filesystem_1.pdf

## file systems locking
* kernel documentation fs locking: https://www.kernel.org/doc/html/latest/filesystems/locking.html

## eVFS
* paper: https://www.usenix.org/system/files/conference/hotstorage18/hotstorage18-paper-sun.pdf
* presentation: https://www.usenix.org/sites/default/files/conference/protected-files/hotstorage18_slides_sun.pdf

## Free blocks
* there is no file system independent solution to obtain the bitmap of the free blocks (they can provide the count, but not their positions): https://stackoverflow.com/questions/10345095/linux-get-free-space-physical-block-numbers-free-space-bitmap
    * kernel space: .statfs superblock hook; kstatds return object with f_bfree parameter, which is a count of the free blocks
    * user space: df command, which causes the kernel to call statfs
    * user space: fsck to add functions to write out a bitmap for each kind of file system, or modify the .statfs hooks to write out the bitmap
    * user space: libext2fs code
    * user space: FIBEMAP / FIBMAP ioctl() can get information about each individual file to collect all used blocks. The free blocks would be the complement of this set of blocks. However, it reports blocks for structures such as journal as free blocks, which would cause the danger of overriding important blocks!!

* user space: github project ext-parser: 
    * https://github.com/tony2037/ext4-parser
    * overview of ext4 layout and file system interactions with devices: https://hackmd.io/@ztex/BkrDCBUXP

* user space command line utilities e2fsprogs: https://en.wikipedia.org/wiki/E2fsprogs
    * debugfs
    * dumpe2fs
    * tune2fs
    * ...

## ext4
* https://www.kernel.org/doc/html/latest/filesystems/ext4/index.html

* really cool commands to show free blocks, block groups, and other file system cool stuff: https://metebalci.com/blog/a-minimum-complete-tutorial-of-linux-ext4-file-system/

* https://selvamvasu.wordpress.com/2014/08/01/inode-vs-ext4/

* https://ext4.wiki.kernel.org/index.php/Ext4_Disk_Layout

* ext4 layout and demonstrations of file creation and deletion: https://www.programmersought.com/article/32394710595/


### using ext4 in the kernel
* how to compile ext4 kernel module: https://www.debugcn.com/en/article/58062672.html

* http://ext4.wiki.kernel.org/index.php/Ext4_Howto

* https://www.kernel.org/doc/Documentation/filesystems/ext4.txt

* https://selvamvasu.wordpress.com/2014/08/01/inode-vs-ext4/

### source code
* https://elixir.bootlin.com/linux/latest/source/fs/ext4/ext4.h

## parsing file system structures
* https://github.com/spullen/EXT3-Parser

# Hooking the kernel
## LSM
* initial paper: http://www.kroah.com/linux/talks/ols_2002_lsm_paper/lsm.pdf

* example of kernel module with LSM (2012): http://blog.ptsecurity.com/2012/09/writing-linux-security-module.html

## file system calls
* intercepting file system calls: https://stackoverflow.com/questions/8588386/intercepting-file-system-system-calls
    * example tpe-lkm: https://github.com/cormander/tpe-lkm
        * https://stackoverflow.com/questions/13995693/new-linux-kernels-no-lsm-using-lkms-no-kernel-hooks-now-what

## syscalls
* https://stackoverflow.com/questions/59584277/linux-driver-development-hooking-open-syscall-leads-to-crash


# Artifice
* Linux Device Mapper that presents the user with a virtual block device. Currently supports FAT-32 and EXT4 file systems. https://www.ssrc.ucsc.edu/proj/Artifice.html

* github source code: https://github.com/atbarker/Artifice


# Time and Polling
* https://www.oreilly.com/library/view/linux-device-drivers/0596005903/ch07.html


# Confused with:
* blk-interposer
* blk-snap
* blk-filter