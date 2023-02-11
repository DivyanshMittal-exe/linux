#include <linux/cst.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/printk.h>
#include <linux/init.h>
// #include <linux/sched/signal.h>
// #include <linux/sched.h>

// static stuct list_head pid_head;

// struct pid_node cst_list_head = {
//     .pid = 0;
//     .next_prev_list =  LIST_HEAD_INIT(list_head.next_prev_list);
// };

// struct list_head cst_list_head = list_head_init(&my_list_head);

// static struct list_head cst_list_head = LIST_HEAD_INIT(cst_list_head);

static LIST_HEAD(cst_list_head);


// struct list_head cst_list_head;

// INIT_LIST_HEAD(&cst_list_head);

// /* Initialize the list head */
// list_head_init(&cst_list_head);

// static LIST_HEAD(&cst_list_head);

/* Initialize the list head */
// INIT_LIST_HEAD(&list_head->next_prev_list);

int  sys_register_impl(pid_t pid)
{
    if (pid < 1)
        return -EINVAL;

    // int pid_exists = 0;
    // struct task_struct *task;

    struct task_struct *task = find_get_task_by_vpid (pid);

    if(!task){
        printk(KERN_INFO "This PID does not exist : %d!\n", pid);
        return -ESRCH;
    }else{

        struct pid_node *node_to_add =	kmalloc(sizeof(struct pid_node), GFP_KERNEL);
        if (!node_to_add) {
            return -ENOMEM;
        }

        node_to_add->pid = pid;
        list_add(&node_to_add->next_prev_list, &cst_list_head);

        printk(KERN_INFO "Yay found  and added : %d!\n", task->pid);


    }
    return 0;

    // rcu_read_lock();
    // for_each_process(task) {
    // 	if (task->pid == pid) {
    // 		// pid_exists = true;
    // 		struct pid_node *node_to_add =
    // 			kmalloc(sizeof(struct pid_node), GFP_KERNEL);
    // 		if (!node_to_add) {
    // 			return -ENOMEM;
    // 		}

    // 		node_to_add->pid = pid;
    // 		list_add(&node_to_add->next_prev_list, &cst_list_head);

    // 		printk(KERN_INFO "Yay found : %d!\n", task->pid);

    // 		return 0;
    // 	} else {
    // 		printk(KERN_INFO "Looking at: %d\n", task->pid);
    // 	}
    // }
    // rcu_read_unlock();

    // return -ESRCH;
}

int  sys_fetch_impl(struct pid_ctxt_switch *stats)
{
    struct pid_node *node;

    struct pid_ctxt_switch *stats_kernel = kmalloc(sizeof(struct pid_ctxt_switch), GFP_KERNEL);






    stats_kernel->ninvctxt = 0;
    stats_kernel->nvctxt = 0;

    list_for_each_entry(node, &cst_list_head, next_prev_list) {
        struct task_struct *task = find_get_task_by_vpid (node->pid);

        if(!task){
            printk(KERN_INFO "This PID does not exist : %d!\n", task->pid);
            return -ESRCH;
        }else{

            printk(KERN_INFO "ninvctxt: %d , nvctxt->%d : %d!\n", task->nivcsw, task->nvcsw, task->pid);


            // get_task_struct(task);
            stats_kernel->ninvctxt += task->nivcsw;
            stats_kernel->nvctxt += task->nvcsw;
            // put_task_struct(task);
        }
    }

    if (copy_to_user(stats, stats_kernel, sizeof(struct pid_ctxt_switch))) {
        return -EFAULT;
    }

    return 0;
}

int  sys_deregister_impl(pid_t pid)
{
    if (pid < 1)
        return -EINVAL;

    struct pid_node *pn;

    list_for_each_entry(pn, &cst_list_head, next_prev_list) {
        if (pn->pid == pid) {
            printk(KERN_INFO "PID removed : %d!\n", pn->pid);

            list_del(&pn->next_prev_list);
            kfree(pn);
            return 0;
        }
    }

    return -ESRCH;
}

SYSCALL_DEFINE1(deregister, pid_t, pid)
{

return sys_deregister_impl(pid);
}


SYSCALL_DEFINE1(fetch, struct pid_ctxt_switch *,stats)
{

return sys_fetch_impl(stats);
}


SYSCALL_DEFINE1(register, pid_t, pid)
{

return sys_register_impl(pid);
}


