#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <asm/current.h>

#define procfs_name "Mythread_info"
#define BUFSIZE  1024
char buf[BUFSIZE];

static ssize_t Mywrite(struct file *fileptr, const char __user *ubuf, size_t buffer_len, loff_t *offset){
    /* Do nothing */
	return 0;
}


static ssize_t Myread(struct file *fileptr, char __user *ubuf, size_t buffer_len, loff_t *offset)
{
    int len = 0;
    struct task_struct *thread = NULL;

    if(*offset > 0){
        return 0;
    }

    for_each_thread(current, thread) {
        if(current->pid == thread->pid){
            continue;
        }
        len += snprintf(buf + len, BUFSIZE - len, "PID: %d, TID: %d, Priority: %d, State: %d\n", thread->tgid, thread->pid, thread->prio, thread->__state);
    }

    int err = copy_to_user(ubuf, buf, len);
    if(err != 0){
        pr_info("Failed to copy data to user space");
        return -EFAULT;
    }

    *offset = len;
    return len;
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