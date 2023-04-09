#include <linux/module.h>  /* Specifically, a module */
#include <linux/kernel.h>  /* We're doing kernel work */
#include <linux/proc_fs.h> /* Necessary because we use the proc fs */
// #include <linux/string.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/kstrtox.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/spinlock.h>

#define procfs_name "sig_target"
#define INTERVAL 1000

static struct timer_list sig_timer;

struct sig_struct
{
    pid_t pid;
    unsigned long int sig;
    struct list_head next_prev_list;
};

// static struct sig_struct sig_stack[max_req_possible];

static LIST_HEAD(sig_list_head);

static struct proc_dir_entry *sig_proc_file;

static DEFINE_SPINLOCK(sig_list_lock);

ssize_t sig_proc_write_handler(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{

    //    char data[count + 1];
    char *data;
    int i;
    int f_part = 0;
    int kstrtoint_ret;
    pid_t pid;
    unsigned int sig_to_call;
    struct sig_struct *t;

    data = kmalloc(count + 1, GFP_KERNEL);
    if (!data)
    {
        printk(KERN_ALERT "Data not allocated: %lu \n", count + 1);
        return -ENOMEM;
    }

    if (copy_from_user(data, buf, count))
    {
        printk(KERN_ALERT "Data not copied\n");
        kfree(data);
        return -EFAULT;
    }

    for (i = 0; i < count; i++)
    {
        if (data[i] == ',')
        {
            data[i] = '\0';
            break;
        }
        ++f_part;
    }
    data[count] = '\0';

    printk(KERN_ALERT "Data is %s\n", data);

    // #10 is base here
    kstrtoint_ret = kstrtoint(data, 10, (int *)&pid);
    if (kstrtoint_ret)
    {
        printk(KERN_ALERT "Data not converted to pid \n");
        kfree(data);
        return kstrtoint_ret;
    }

    // #10 is base here
    kstrtoint_ret = kstrtouint(&data[f_part + 2], 10, &sig_to_call);
    if (kstrtoint_ret)
    {
        printk(KERN_ALERT "Data not converted to signal\n");
        kfree(data);
        return kstrtoint_ret;
    }

    t = kmalloc(sizeof(struct sig_struct), GFP_KERNEL);
    if (!t)
    {
        return -ENOMEM;
    }

    t->pid = pid;
    t->sig = sig_to_call;

    spin_lock(&sig_list_lock);

    list_add(&t->next_prev_list, &sig_list_head);

    printk(KERN_INFO "Placed in the list (%d,%ld)\n", t->pid, t->sig);

    spin_unlock(&sig_list_lock);

    return count;
}

static const struct proc_ops sig_proc_ops = {
    .proc_write = sig_proc_write_handler,
};

void actual_signal_handlers(struct timer_list *t_l)
{

    struct task_struct *task;
    struct sig_struct *pn, *next;

    spin_lock(&sig_list_lock);

    list_for_each_entry_safe(pn, next, &sig_list_head, next_prev_list)
    {

        // printk(KERN_INFO "Sent %d the sig %lu\n", pn->pid, pn->sig);

        task = pid_task(find_vpid(pn->pid), PIDTYPE_PID);
        if (!task)
        {
            printk(KERN_INFO "This PID does not exist : %d!\n", pn->pid);
        }
        else
        {
            send_sig(pn->sig, task, 0);
            printk(KERN_INFO "Sent %d the sig %lu\n", pn->pid, pn->sig);
        }

        list_del(&pn->next_prev_list);
        kfree(pn);
    }

    spin_unlock(&sig_list_lock);

    mod_timer(&sig_timer, jiffies + msecs_to_jiffies(INTERVAL));
}

static int __init sig_init(void)
{
    //                                                 All have write permssions
    sig_proc_file = proc_create(procfs_name, 0666, NULL, &sig_proc_ops);

    if (!sig_proc_file)
    {
        printk(KERN_ALERT "Error: Could not initialize /proc/%s\n", procfs_name);
        return -ENOMEM;
    }

    printk(KERN_INFO "/proc/%s created\n", procfs_name);

    sig_timer.expires = jiffies + msecs_to_jiffies(INTERVAL);
    sig_timer.function = actual_signal_handlers;
    add_timer(&sig_timer);

    return 0;
}

static void __exit sig_exit(void)
{
    struct sig_struct *pn, *next;

    del_timer(&sig_timer);
    remove_proc_entry(procfs_name, NULL);
    printk(KERN_INFO "/proc/%s removed\n", procfs_name);

    list_for_each_entry_safe(pn, next, &sig_list_head, next_prev_list)
    {
        list_del(&pn->next_prev_list);
        kfree(pn);
    }
}

module_init(sig_init);
module_exit(sig_exit);

MODULE_AUTHOR("Divyansh Mittal");
MODULE_LICENSE("GPL");