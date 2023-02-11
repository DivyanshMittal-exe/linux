#include <linux/module.h>	/* Specifically, a module */
#include <linux/kernel.h>	/* We're doing kernel work */
#include <linux/proc_fs.h>	/* Necessary because we use the proc fs */
// #include <linux/string.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/kstrtox.h>

#define procfs_name "sig_target"
#define PROCFS_MAX_SIZE 	2048


MODULE_AUTHOR("Divyansh Mittal");
MODULE_LICENSE("GPL");

ssize_t	sig_proc_write_handler(struct file *file, const char __user *buf, size_t count, loff_t *ppos){
    char data[count + 1];
    int f_part = 0;

    if (copy_from_user(data, buf, count))
        return -EFAULT;

    

    for (i = 0; i < count; i++) {
        if(data[i] == ','){
            break;
        }
        ++f_part;
    }

    data[f_part] = '\0';

    data[count] = '\0';


    int pid_size = f_part;
    int sig_call_starts_at = f_part + 2;
    int sig_call_size = count - f_part - 2;

    int kstrtoint_ret;
    pid_t pid;

    kstrtoint_ret = kstrtouint(data, pid_size, (unsigned int *) &pid);
    if (kstrtoint_ret){
        return kstrtoint_ret;
    }

    unsigned int sig_to_call;

    kstrtoint_ret = kstrtouint(&data[sig_call_starts_at], sig_call_size, &sig_to_call);
    if (kstrtoint_ret){
        return kstrtoint_ret;
    }

    int kill_ret;
    kill_ret = kill_pid(find_get_task_by_vpid(pid), sig_to_call, 1);

    if (kill_ret)
        return kill_ret;

    return count;

    

}


static struct proc_dir_entry *sig_proc_file;

static const struct proc_ops sig_proc_ops = {
        .proc_weite = sig_proc_write_handler,
    };

int init_module()
{
//                                                 All have write permssions
    sig_proc_file = proc_create(procfs_name, 0666, NULL,&sig_proc_ops);

    /* check if the /proc file was created successfuly */
    if (!sig_proc_file){
        printk(KERN_ALERT "Error: Could not initialize /proc/%s\n", proc_dir_entry);
        return -ENOMEM;
    }

    printk(KERN_INFO "/proc/%s created\n", proc_dir_entry);

    return 0;
}

void cleanup_module()
{
    remove_proc_entry(proc_dir_entry, &proc_root);
    printk(KERN_INFO "/proc/%s removed\n", proc_dir_entry);
}