#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <asm/current.h>

#define procfs_name "Mythread_info"
#define BUFSIZE  1024
char buf[BUFSIZE]; //kernel buffer

static ssize_t Mywrite(struct file *fileptr, const char __user *ubuf, size_t buffer_len, loff_t *offset){
    /*Your code here*/
    if(*offset > 0) {
        return 0;
    }

    if(copy_from_user(buf, ubuf, buffer_len)) {
        return -EFAULT;
    }
    buf[buffer_len] = '\0';

    *offset = buffer_len+1;
    return buffer_len+1;
    /****************/
}


static ssize_t Myread(struct file *fileptr, char __user *ubuf, size_t buffer_len, loff_t *offset){
    /*Your code here*/
    if(*offset > 0) {
        return 0;
    }

    int len = snprintf(buf, BUFSIZE, 
                    "PID: %d, TID: %d, time: %llu\n", 
                    current->tgid, current->pid, current->utime/100/1000);

    int err = copy_to_user(ubuf, buf, buffer_len);
    if(err != 0) {
        return -EFAULT;
    }
    
    *offset = len;
    return len;
    /****************/
}

static struct proc_ops Myops = {
    .proc_read = Myread,
    .proc_write = Mywrite,
};

static int My_Kernel_Init(void){
    proc_create(procfs_name, 0644, NULL, &Myops);   
    pr_info("My kernel says Hi");
    return 0;
}

static void My_Kernel_Exit(void){
    pr_info("My kernel says GOODBYE");
}

module_init(My_Kernel_Init);
module_exit(My_Kernel_Exit);

MODULE_LICENSE("GPL");
