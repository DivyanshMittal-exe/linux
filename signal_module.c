#include <linux/module.h>	/* Specifically, a module */
#include <linux/kernel.h>	/* We're doing kernel work */
#include <linux/proc_fs.h>	/* Necessary because we use the proc fs */
// #include <linux/string.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/kstrtox.h>
#include <linux/slab.h>
#include <linux/sched.h>

#define procfs_name "sig_target"
#define PROCFS_MAX_SIZE 	2048

static struct proc_dir_entry *sig_proc_file;


static DEFINE_SPINLOCK(sig_proc_lock);

unsigned long flags;




MODULE_AUTHOR("Divyansh Mittal");
MODULE_LICENSE("GPL");

ssize_t	sig_proc_write_handler(struct file *file, const char __user *buf, size_t count, loff_t *ppos){


//    char data[count + 1];
    char* data;
    int i;
    int f_part = 0;
    int kstrtoint_ret;
    pid_t pid;
    int pid_size;
    int sig_call_starts_at;
    int sig_call_size;
    struct task_struct *task;

    unsigned int sig_to_call;


    spin_lock_irqsave(&sig_proc_lock, flags);


    data = kmalloc(count + 1, GFP_KERNEL);
    if (!data) {
        printk(KERN_ALERT "Data not allocated: %lu \n", count + 1);
        spin_unlock_irqrestore(&sig_proc_lock, flags);

        return -ENOMEM;
    }



    if (copy_from_user(data, buf, count)){
        printk(KERN_ALERT "Data not copied\n");
        kfree(data);
        spin_unlock_irqrestore(&sig_proc_lock, flags);

        return -EFAULT;

    }


    for ( i = 0; i < count; i++) {
        if(data[i] == ','){
            break;
        }
        ++f_part;
    }

    data[f_part] = '\0';

    data[count] = '\0';


    pid_size = f_part;
    sig_call_starts_at = f_part + 2;
    sig_call_size = count - f_part - 2;

    printk(KERN_ALERT "Data is %s\n",data);

//#10 is base here
    kstrtoint_ret = kstrtoint(data, 10, (int *) &pid);
    if (kstrtoint_ret){
        printk(KERN_ALERT "Data not converted to pid \n");

        kfree(data);
        spin_unlock_irqrestore(&sig_proc_lock, flags);

        return kstrtoint_ret;
    }

//#10 is base here
    kstrtoint_ret = kstrtouint(&data[sig_call_starts_at], 10, &sig_to_call);
    if (kstrtoint_ret){
        printk(KERN_ALERT "Data not converted to signal\n");

        kfree(data);
        spin_unlock_irqrestore(&sig_proc_lock, flags);

        return kstrtoint_ret;
    }

//    kill_ret = kill_pid(find_vpid(pid), sig_to_call, 1);

    rcu_read_lock();
    task = pid_task(find_vpid(pid), PIDTYPE_PID);
    rcu_read_unlock();


//    task = find_get_task_by_vpid (pid);
    if (!task) {
        printk(KERN_ALERT "Task does not exist\n");
        kfree(data);
        spin_unlock_irqrestore(&sig_proc_lock, flags);

        return -ESRCH;
    }
//#0 is privelege level, here unpriveleged
    send_sig(sig_to_call,task,0);


    printk(KERN_INFO "Sent %d the sig %du\n", pid, sig_to_call);

    kfree(data);
    spin_unlock_irqrestore(&sig_proc_lock, flags);



    return count;

}

static const struct proc_ops sig_proc_ops = {
        .proc_write = sig_proc_write_handler,
};


int init_module()
{
//                                                 All have write permssions
    sig_proc_file = proc_create(procfs_name, 0666, NULL,&sig_proc_ops);

    /* check if the /proc file was created successfuly */
    if (!sig_proc_file){
        printk(KERN_ALERT "Error: Could not initialize /proc/%s\n", procfs_name);
        return -ENOMEM;
    }

    printk(KERN_INFO "/proc/%s created\n", procfs_name);

    return 0;
}

void cleanup_module()
{
    remove_proc_entry(procfs_name, sig_proc_file);
    printk(KERN_INFO "/proc/%s removed\n", procfs_name);
}