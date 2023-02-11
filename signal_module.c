#include <linux/module.h>	/* Specifically, a module */
#include <linux/kernel.h>	/* We're doing kernel work */
#include <linux/proc_fs.h>	/* Necessary because we use the proc fs */

#define procfs_name "sig_target"
#define PROCFS_MAX_SIZE 	2048


MODULE_AUTHOR("Divyansh Mittal");
MODULE_LICENSE("GPL");


static struct proc_dir_entry *sig_proc_file;

static const struct proc_ops sig_proc_ops = {
        .proc_open = skynet_open,
        .proc_read = seq_read,
        .proc_lseek = seq_lseek,
        .proc_release = single_release,
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