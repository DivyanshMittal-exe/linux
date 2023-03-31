#include <linux/cst.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/printk.h>
#include <linux/init.h>

#include <linux/sched.h>

// static LIST_HEAD(cst_list_head);

SYSCALL_DEFINE4(register_rm, pid_t, pid,int,period,int,deadline,int,exec_time){

	if (pid < 1)
		return -EINVAL;

	struct task_struct *task = find_get_task_by_vpid (pid);
	struct sched_param param = { .sched_priority = 50 };

	struct sched_rm_entity * rm_of_task = &(task->rm);
	rm_of_task->dl_runtime = exec_time;
	rm_of_task->dl_deadline = deadline;
	rm_of_task->dl_period = period;


	rm_of_task->runtime = exec_time;
	rm_of_task->deadline = exec_time;

	sched_setscheduler_nocheck(struct task_struct *, int, const struct sched_param *);


}

SYSCALL_DEFINE4(register_dm, pid_t, pid,int,period,int,deadline,int,exec_time){

	if (pid < 1)
		return -EINVAL;

}

SYSCALL_DEFINE1(yield, pid_t, pid){

	if (pid < 1)
		return -EINVAL;

}

SYSCALL_DEFINE1(remove, pid_t, pid){
	if (pid < 1)
		return -EINVAL;

}

SYSCALL_DEFINE0(list){

}




