#include <linux/kernel.h>

#define BUF_SIZE 50 /*Number of bytes*/ 
char *buf; /*Global Variable*/


static int hello_init(void) { 

    printk(KERN_INFO "Hello world.\n"); /* Memory allocation*/
    buf = kmalloc(BUF_SIZE, GFP_KERNEL); 
    
    if (!buf)  
        return -ENOMEM; 
    
    return 0;
}


static void hello_exit(void) {
    
    printk(KERN_INFO "Goodbye world.\n"); 
    
    if (buf) {
        /*freeing memory*/
        kfree(buf); 
    }
} 

module_init(hello_init); 
module_exit(hello_exit); 
   