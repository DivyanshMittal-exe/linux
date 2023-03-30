#include <linux/cst.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/printk.h>
#include <linux/init.h>


// static LIST_HEAD(cst_list_head);

SYSCALL_DEFINE4(register_rm, pid_t, pid,int,period,int,deadline,int,exec_time){

	if (pid < 1)
		return -EINVAL;


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




