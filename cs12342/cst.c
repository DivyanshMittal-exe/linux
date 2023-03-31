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

#include <linux/ktime.h>

#include <linux/rbtree.h>

#include <linux/jiffies.h>

// static LIST_HEAD(cst_list_head);

struct rb_root_cached runnables = RB_ROOT_CACHED;

static LIST_HEAD(all_processes);

#define  cpu_to_run_on  0;
#define prio_of_process  32;

struct rm_entity{

	struct rb_node rb_nd;

	struct list_head list_nd;

	struct task_struct *p;

	struct timer_list callback_timer;

	u64				dl_runtime;	/* Maximum runtime for each instance	*/
	u64				dl_deadline;	/* Relative deadline of each instance	*/
	u64				dl_period;	/* Separation of two instances (period) */
	u64				dl_bw;		/* dl_runtime / dl_period		*/
	u64				dl_density;	/* dl_runtime / dl_deadline		*/


	s64				runtime;	/* Remaining runtime for this instance	*/
	u64				deadline;	/* Absolute deadline for this instance	*/
	int				flags;		/* Specifying the scheduler behaviour	*/


};


static  bool __rm_less(struct rb_node *a, const struct rb_node *b)
{
	stuct sched_rm_entity *f =   rb_entry((a), struct rm_entity, rb_nd);
	stuct sched_rm_entity *s =   rb_entry((b), struct rm_entity, rb_nd);
	return f->deadline < s->deadline;
}






static void callback_deadline_setter(unsigned long data)
{

	struct rm_entity *entity = (struct rm_entity *)data;
	if(entity->flags == 1){
		printk(KERN_INFO "Exceeded Deadline limit %d\n", pid_nr((entity->p)->pid));
		rb_erase_cached(&(entity->rb_nd),&(runnables));

	}else{
		entity->flags = 1;
	}

	entity->deadline += entity->dl_deadline;

	rb_add_cached(&(entity->rb_nd), &runnables , __rm_less);


}





void __schedule_rm(){

	struct task_struct* curr_task = cpu_curr(cpu_to_run_on);

	struct rm_entity *curr_entity;
	struct task_struct *task;

	list_for_each_entry(curr_entity, &all_processes, list_nd) {
		task = curr_entity->p;
		if (task && task->pid == curr_task->pid) {
			printk(KERN_INFO "Found rm_entity with pid %d\n", pid);
			break;
		}
	};




	struct rb_node* leftmost = rb_first_cached();
	struct rm_entity* leftmost_rm_entity= container_of(leftmost, struct rm_entity, rb_nd);
	struct task_struct* leftmost_task_struct = leftmost_rm_entity->p;

	char state_of_curr_task = task_state_to_char(curr_task->state);

	if(state_of_curr_task != 'R' || curr_entity->flags != 1 || curr_entity->deadline > leftmost_rm_entity->deadline){

		if (kill(curr_task->pid, SIGSTOP) == -1) {
			perror("kill");
			return -1;
		}

		if (kill(leftmost_task_struct->pid, SIGCONT) == -1) {
			perror("kill");
			return -1;
		}


	}

}




int rm_dm_implementation( pid_t pid,u64 period,u64 deadline,u64 exec_time){

	if (pid < 1)
		return -EINVAL;


	struct rm_entity *rm_of_task = kmalloc(sizeof(struct sig_struct), GFP_KERNEL);

	struct task_struct *task = find_get_task_by_vpid (pid);

	rm_of_task->p = task;
	rm_of_task->dl_runtime = exec_time;
	rm_of_task->dl_deadline = deadline;
	rm_of_task->dl_period = period;

	rm_of_task->flags = 1;


	rm_of_task->runtime = exec_time;
	rm_of_task->deadline = jiffies + deadline;


	rb_add_cached(&(rm_of_task->rb_nd), &runnables , __rm_less);

	list_add(&(rm_of_task->list_nd), &all_processes);

	struct sched_param param;
	param.sched_priority = prio_of_process;


	if (sched_setscheduler(task, SCHED_FIFO, &param) == -1) {
		perror("sched_setscheduler");
		return -1;
	}

	if (kill(pid, SIGSTOP) == -1) {
		perror("kill");
		return -1;
	}

	setup_timer(&(rm_of_task->callback_timer), callback_deadline_setter, (unsigned long)&rm_of_task);

	mod_timer(&(rm_of_task->callback_timer), jiffies + msecs_to_jiffies(deadline));

	__schedule_rm();

	return 0;

}


SYSCALL_DEFINE4(register_rm, pid_t, pid,u64,period,u64,deadline,u64,exec_time){


	return rm_dm_implementation(pid,period,deadline,exec_time);

}

SYSCALL_DEFINE4(register_dm, pid_t, pid,u64,period,u64,deadline,u64,exec_time){

	if (pid < 1)
		return -EINVAL;

	return rm_dm_implementation(pid,period,deadline,exec_time);


}

SYSCALL_DEFINE1(yield, pid_t, pid){

	if (pid < 1)
		return -EINVAL;

	struct rm_entity *entity;
	struct task_struct *task;

	list_for_each_entry(entity, &all_processes, list_nd) {
		task = entity->p;
		if (task && task->pid == pid) {
			printk(KERN_INFO "Found rm_entity with pid %d\n", pid);
			entity->flags = 0;

			rb_erase_cached(&(entity->rb_nd),&(runnables));

			if (kill(pid, SIGSTOP) == -1) {
				perror("kill");
				return -1;
			}

			break;
		}
	}


	__schedule_rm();

	return 0;

}

SYSCALL_DEFINE1(remove, pid_t, pid){
	if (pid < 1)
		return -EINVAL;

	struct rm_entity *entity;
	struct task_struct *task;

	list_for_each_entry(entity, &all_processes, list_nd) {
		task = entity->p;
		if (task && task->pid == pid) {

			rb_erase_cached(&(entity->rb_nd),&(runnables));
			list_del(&(entity->list_nd));

			entity->flags = 0;

			struct sched_param param;
			param.sched_priority = 0;


			if (sched_setscheduler(task, SCHED_OTHER, &param) == -1) {
				perror("sched_setscheduler");
				return -1;
			}


			del_timer(&(rm_of_task->callback_timer));


			printk(KERN_INFO "Found rm_entity with pid %d\n", pid);
			break;
		}
	};


	return 0;

}

SYSCALL_DEFINE0(list){

	struct rm_entity *entity;
	printf("Printing process information...\n");

	list_for_each_entry(entity, &all_processes, list_nd) {
		printf("PID: %d, period: %llu, deadline: %llu, execution time: %lld\n",
		       entity->p->pid, entity->dl_period, entity->dl_deadline, entity->dl_runtime);
	}


}




SYSCALL_DEFINE2(resource_map, pid_t, pid, unsigned int, RID){}
SYSCALL_DEFINE1(start_pcp,unsigned int, RID){}
SYSCALL_DEFINE2(pcp_lock, pid_t, pid, unsigned int, RID){}
SYSCALL_DEFINE2(pcp_unlock, pid_t, pid, unsigned int, RID){}







