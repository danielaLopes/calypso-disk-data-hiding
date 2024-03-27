# Basic functions
* see local file basics.pdf
* defined in linux/slab.h

## Allocate and free
```
/*
 * Allocate memory
 *  size is the size of the block to be allocated
 *  flags control the behaviour of kmalloc:
 *     For instance, GFP_KERNEL means the allocation is performed on behalf of a process running in kernel space. this means that the calling function is executing a system call on behalf of a process. So this can put the current process to sleep, waiting for a page when called in a low-memory situation
 */
void *kmalloc(size_t size, int flags);

/*
 * Free memory
 */
void kfree(void *ptr);

```

## container_of
* https://www.stupid-projects.com/container_of/