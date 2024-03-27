/*
* current_proc.c
*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>


int read_proc(char *buf,char **start,off_t offset,int count,int *eof,void *data )
{
int len=0;
 len = sprintf(buf,"\n Name :%s \n Process id: %d\n ",current->comm,current->pid);
printk(KERN_INFO "Inside the proc entry");
return len;
}

void create_new_proc_entry()
{
create_proc_read_entry("process_data",0,NULL,read_proc,NULL);

}


int char_arr_init (void) {
   
    create_new_proc_entry();
    return 0;
}

void char_arr_cleanup(void) {
    printk(KERN_INFO " Inside cleanup_module\n");
    remove_proc_entry("process_data",NULL);
}
MODULE_LICENSE("GPL");   
module_init(char_arr_init);
module_exit(char_arr_cleanup);