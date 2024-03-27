#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/errno.h>   /* error codes */
#include <linux/sched.h>
 
 
MODULE_LICENSE("Dual BSD/GPL");
 
/* Declaration of functions */
void device_exit(void);
int device_init(void);
   
/* Declaration of the init and exit routines */
module_init(device_init);
module_exit(device_exit);
 
int device_init(void)
{
    struct task_struct *task = current; // getting global current pointer
    printk(KERN_NOTICE "assignment: current process: %s, PID: %d", task->comm, task->pid);
    do
    {
        task = task->parent; 
        // all processes running are linked in a list that can be accessed with task_struct->parent
        printk(KERN_NOTICE "assignment: parent process: %s, PID: %d", task->comm, task->pid);
    } while (task->pid != 0);
    return 0;
}
 
void device_exit(void) {
  printk(KERN_NOTICE "assignment: exiting module");
}