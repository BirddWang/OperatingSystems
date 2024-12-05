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
    if(copy_from_user(buf, ubuf, buffer_len)) {
        return -EFAULT;
    }
    buf[buffer_len] = '\n';
    buffer_len += 1;
    buffer_len += snprintf(buf+buffer_len, BUFSIZE, 
                    "PID: %d, TID: %d, time: %llu\n", 
                    current->tgid, current->pid, current->utime/100/1000);

    pr_info("Write: %s\n", buf);
    return buffer_len;
}


static ssize_t Myread(struct file *fileptr, char __user *ubuf, size_t buffer_len, loff_t *offset){
    pr_info("Read Entry\n");
    if (*offset >= buffer_len){
        return 0;
    }

    size_t remain = buffer_len - *offset;
    size_t copy_len = remain < BUFSIZE ? remain : BUFSIZE;
    if(copy_to_user(ubuf, buf+*offset, copy_len)) {
        return -EFAULT;
    }
    *offset += copy_len;
    pr_info("Read: %s\n", buf);
    return copy_len;
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
    remove_proc_entry(procfs_name, NULL);
    pr_info("My kernel says GOODBYE");
}

module_init(My_Kernel_Init);
module_exit(My_Kernel_Exit);

MODULE_LICENSE("GPL");
