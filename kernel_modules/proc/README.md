# Documentation
* http://www.cs.albany.edu/~sdc/CSI500/linux-2.6.31.14/Documentation/DocBook/procfs-guide/index.html
* Really good recent guide: https://devarea.com/linux-kernel-development-creating-a-proc-file-and-interfacing-with-user-space/#.YBwas2T7Ta5
* Permission denied /proc file: https://stackoverflow.com/questions/20977085/permission-denied-error-in-reading-proc-file
* https://lkw.readthedocs.io/en/latest/doc/05_proc_interface.html
* Correct implementation of 4 main operations: https://www.linuxtopia.org/online_books/Linux_Kernel_Module_Programming_Guide/x814.html
* Updated version of the guide: 
    * https://wiki.tldp.org/lkmpg/en/content/ch05/2_6
    * https://tldp.org/LDP/lkmpg/2.4/html/c768.htm (also includes adaptations for several kernel versions)
* Current process: 
    * https://tuxthink.blogspot.com/2011/04/current-process.html
    * https://www.humblec.com/retrieving-current-processtask_struct-in-linux-kernel/
* /proc files: https://web.mit.edu/rhel-doc/5/RHEL-5-manual/Deployment_Guide-en-US/s1-proc-topfiles.html


# Observations
* Cannot load two modules that create the same /proc file: insmod: ERROR: could not insert module ./procfs_hello.ko: Cannot allocate memory


# Basic functions and structures to create and managing /proc filesystem entry
* https://developer.ibm.com/articles/l-proc/
* Observation, this is outdated for recent kernels, but is a good explanation 

## Creating and removing a /proc entry
```
/* create virtual file in /proc filesystem
 * accepts a file name, set of permissions and a location in /proc for the file to reside
 * 
 * returns a pointer to a proc_dir_entry or NULL in case of error
 */
struct proc_dir_entry *proc_create(const char *name, /*The name of the proc entry*/
                                umode_t mode, /*The access mode for proc entry*/
                                struct proc_dir_entry *parent, /*The name of the parent directory under /
proc*/
                                const struct file_operations *proc_fops /*The structure in which the file
operations for the proc entry will be created*/
)


/*
 * Remove a file from /proc
 * provide the file name string and the location of the file in /proc (it's parent). 
 *  the parent argument can be NULL or proc_root_fs to create the file in /proc
 *  proc_net for /proc/net
 *  proc_bus for /proc/bus
 *  proc_root_driver for /proc/driver
 */
void remove_proc_entry( const char *name, struct proc_dir_entry *parent );

/*
 * Can be used ro configure aspects of the virtual file created in /proc,
 * such as the function call when a read is performed on the file
 */
struct proc_dir_entry {

    const char *name;            // virtual file name

    mode_t mode;                // mode permissions

    uid_t uid;                // File's user id

    gid_t gid;                // File's group id

    struct inode_operations *proc_iops;    // Inode operations functions

    struct file_operations *proc_fops;    // File operations functions

    struct proc_dir_entry *parent;        // Parent directory

    ...

    read_proc_t *read_proc;            // /proc read function

    write_proc_t *write_proc;        // /proc write function

    void *data;                // Pointer to private data

    atomic_t count;                // use count

    ...

};
```

## Structures
```
/*
 * It is a collection of function pointers.
 * Each open file is associated with its own set of functions
 */
struct file_operations {
    struct module *owner // It is a pointer to the module that “owns” the structure; This field is used to prevent the module from being unloaded while its operations are in use; Almost all the time, it is simply initialized to THIS_MODULE.

    ssize_t (*read) (struct file *, char __user *,
        size_t, loff_t *) // It is used to retrieve data from the kernel; A non negative return value represents the number of bytes successfully read.

    ssize_t (*write) (struct file *, const char
        __user *, size_t, loff_t *) // It writes( or sends) data to the kernel; The return value, if non-negative, represents the number of bytes successfully written.

    int (*open) (struct inode *, struct file *) // This is always the first operation performed on the file structure

    int (*release) (struct inode *, struct file *) // 
This operation is invoked when the file structure is being released
};       
```

## write_proc callback
```
/*
 * Overwrites the callback function to write to a /proc/entry
 *  buff is the string data being passed to the file
 *  the buffer address is a user-space buffer, so we are not able to read it directly, so we use copy_from_user()
 *  len defines how much data in buff is being written
 *  the data argument is a pointer to private data
 */
int mod_write( struct file *filp, const char __user *buff,
               unsigned long len, void *data );
```

## read_proc callback
```
/*
 * Overwrites the callback function to read from a /proc/entry
 *  the page is the location into which you write the data intended for the user
 *  count defines the maximum number of characters that can be written
 *  start and off are used when returning to more than a page of data (typically 4KB)
 *  when all the data has been written, set the eof (end-of-file) argument
 *  as you write, data represents private data
 *  the page buffer provided is in the kernel space, so we can write to it without invoking copy_to_user()
 */
int mod_read( char *page, char **start, off_t off, 
              int count, int *eof, void *data );
```

## other useful functions
```
/* Create a directory in the proc filesystem */
struct proc_dir_entry *proc_mkdir( const char *name,
                                     struct proc_dir_entry *parent );

/* Create a symlink in the proc filesystem */
struct proc_dir_entry *proc_symlink( const char *name,
                                       struct proc_dir_entry *parent,
                                       const char *dest );

/* Create a proc_dir_entry with a read_proc_t in one call 
   for entries that only require a read function */
struct proc_dir_entry *create_proc_read_entry( const char *name,
                                                  mode_t mode,
                                                  struct proc_dir_entry *base,
                                                  read_proc_t *read_proc,
                                                  void *data );

/* Copy buffer to user-space from kernel-space */
unsigned long copy_to_user( void __user *to,
                              const void *from,
                              unsigned long n );

/* Copy buffer to kernel-space from user-space */
unsigned long copy_from_user( void *to,
                                const void __user *from,
                                unsigned long n );

/* Allocate a 'virtually' contiguous block of memory */
void *vmalloc( unsigned long size );

/* Free a vmalloc'd block of memory */
void vfree( void *addr );

/* Export a symbol to the kernel (make it visible to the kernel) */
EXPORT_SYMBOL( symbol );

/* Export all symbols in a file to the kernel (declare before module.h) */
EXPORT_SYMTAB
``` 


# File permissions flags and values
## References
*  https://stackoverflow.com/questions/28664971/why-is-does-proc-create-mode-argument-0-correspond-to-0444
* https://pubs.opengroup.org/onlinepubs/7908799/xsh/sysstat.h.html
* https://www.gnu.org/software/libc/manual/html_node/Permission-Bits.html
* https://stackoverflow.com/questions/40163270/what-is-s-isreg-and-what-does-it-do

## Mode bits for access permission
### File owner user
* S_IRUSR or S_IREAD: read permission for owner
* S_IWUSR or S_IWRITE: write permission for owner
* S_IXUSR or S_IEXEC: execute/search permission for owner

* S_IRWXU: (S_IRUSR | S_IWUSR | S_IXUSR)

### File owner group
* S_IRGRP: read permission for group
* S_IWGRP: write permission for group
* S_IXGRP: execute/search permission for group

* S_IRWXG: (S_IRGRP | S_IWGRP | S_IXGRP)

### Other users
* S_IROTH: read permission for other users
* S_IWOTH: write permission for other users
* S_IXOTH: execute/search permission for other users

* S_IRWXO: (S_IROTH | S_IWOTH | S_IXOTH)

### Set ID
* S_ISUID: set-user-ID
* S_ISGID: set-group-ID
* S_ISVTX: sticky bit, only for directories, restricted deletion flag

## Allow for test of file type
* S_IFMT: type of file
* S_IFBLK: block special
* S_IFCHR: character special
* S_IFIFO: FIFO special
* S_IFREG: regular
* S_IFDIR: directory
* S_IFLNK: symbolic link


## Structure of permission
* https://developer.ibm.com/technologies/linux/tutorials/l-lpic1-104-5/

* fuuugggooot.: read write execute bit for user owner (u), group (g) or other users (o). (f) is for the type of file, for example a directory is d, a regular file is -; the (.) represents whether there is an alternate access method. It can be a space, so it might not be noticeable. (t) is the sticky bit, that only permits the owner of the file or root to delete or unlink a file if set, instead of everyone with write permission.

* permissions in each group can be applied with either symbols or octal numbers:
    * rwx or 7
    * rw- or 6
    * r-x or 5
    * r-- or 4
    * -wx or 3
    * -w- or 2
    * --x or 1
    * --- or 0

* suid and sgid: two special access modes called suid (set user id) and sgid (set group id). When an executable program has the suid access modes set, it will run as if it had been started by the file's owner, rather than by the user who really started it. Similarly, with the sgid access modes set, the program will run as if the initiating user belonged to the file's group rather than to his own group.
    * u+s or 4000 sets suid
    * g+s or 2000 sets sgid
    * u-s unsets suid
    * g-s unsets sgid
    * s replaces the x for execute mode
    * sticky: t or 1000

* in octal, every character corresponds to three bits. For instance, 6 is 110.
    * http://permissions-calculator.org/
    * e.g: chmod 1777: sticky bit and rwx permission for all users
    * 4 characters, left is for special (setuid, setgid or sticky bit), then it's for user owner (r,w,x), then for group, and then for others