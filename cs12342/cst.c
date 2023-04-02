#include <linux/cst.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/printk.h>
#include <linux/init.h>

#include <linux/pid.h>

#include <linux/kernel.h>

#include <linux/sched/task.h>
#include <linux/sched/types.h>
//#include <linux/sched/prio.h>
//#include <linux/sched/rt.h>

#include <linux/syscalls.h>


#include <uapi/linux/sched/types.h>

#include <linux/sched/signal.h>

#include <linux/sched.h>

#include <linux/ktime.h>

#include <linux/rbtree.h>

#include <linux/jiffies.h>

// static LIST_HEAD(cst_list_head);

struct rb_root_cached runnables = RB_ROOT_CACHED;

static LIST_HEAD(all_processes);

static DEFINE_SPINLOCK(sched_lock);


struct rm_entity *last_run_by_me = NULL;

#define  cpu_to_run_on  0;
#define prio_of_process  32;

struct rm_entity{

	struct rb_node rb_nd;

	struct list_head list_nd;

	struct task_struct *p;

	struct timer_list callback_timer;

	u64  	 			dl_runtime;	/* Maximum runtime for each instance	*/
	u64				dl_deadline;	/* Relative deadline of each instance	*/
	u64 				dl_period;	/* Separation of two instances (period) */
	u64 				dl_bw;		/* dl_runtime / dl_period		*/
	u64 				dl_density;	/* dl_runtime / dl_deadline		*/


	u64 				dl_priority;


	s64				runtime;	/* Remaining runtime for this instance	*/
	u64 				deadline;	/* Absolute deadline for this instance	*/
	int				flags;		/* Specifying the scheduler behaviour	*/


};


static  bool __rm_less_rms(struct rb_node *a, const struct rb_node *b)
{
	struct rm_entity *f =   rb_entry((a), struct rm_entity, rb_nd);
	struct rm_entity *s =   rb_entry((b), struct rm_entity, rb_nd);
	return f->dl_deadline < s->dl_deadline;
}






void __schedule_rm(void){

	struct rm_entity* curr_entity = last_run_by_me;


	if(RB_EMPTY_ROOT(&(runnables.rb_root))){
		return ;
	}

	if(!curr_entity){
		struct rb_node* leftmost;
		struct rm_entity* leftmost_rm_entity;
		leftmost = rb_first_cached(&runnables);
		struct task_struct* leftmost_task_struct;
		leftmost_rm_entity= container_of(leftmost, struct rm_entity, rb_nd);
		leftmost_task_struct = leftmost_rm_entity->p;

		printk(KERN_INFO,"First Process added %d\n", leftmost_task_struct->pid);

		if (send_sig(SIGCONT, leftmost_task_struct, 0) < 0) {
			printk(KERN_INFO "send_sig SIGCONT failed for task %d\n", leftmost_task_struct->pid);
			return;
		}

		last_run_by_me = leftmost_rm_entity;
//		leftmost_rm_entity->flags = 1;
		return ;
	}


	struct task_struct *curr_task = last_run_by_me->p;




	struct rb_node* leftmost;
	struct rm_entity* leftmost_rm_entity;
	struct task_struct* leftmost_task_struct;
	char state_of_curr_task;


	leftmost = rb_first_cached(&runnables);
	leftmost_rm_entity= container_of(leftmost, struct rm_entity, rb_nd);
	leftmost_task_struct = leftmost_rm_entity->p;

	state_of_curr_task = task_state_to_char(curr_task);


	printk(KERN_INFO "Currently running %d, with deadline %llu, leftmost is %d, with deadline %llu\n", curr_task->pid, last_run_by_me->dl_deadline, leftmost_rm_entity->p->pid, leftmost_rm_entity->dl_deadline);


	if(state_of_curr_task != 'R' || curr_entity->flags != 1 || curr_entity->dl_deadline > leftmost_rm_entity->dl_deadline){


		if (send_sig(SIGSTOP, curr_task, 0) < 0) {

//			curr_entity->flags = 0;

			printk(KERN_INFO "send_sig SIGSTOP failed for task %d\n", curr_task->pid);
			return;
		}

		if (send_sig(SIGCONT, leftmost_task_struct, 0) < 0) {
//			leftmost_rm_entity->flags = 1;
			printk(KERN_INFO "send_sig SIGCONT failed for task %d\n", leftmost_task_struct->pid);
			return;
		}

		last_run_by_me = leftmost_rm_entity;
	}

	printk(KERN_INFO "So I schedule %d, with deadline %lld\n", last_run_by_me->p->pid, last_run_by_me->dl_deadline);




}



void callback_deadline_setter(struct timer_list *t_l)
{

	//spin_lock(&sched_lock);


	//	struct rm_entity *entity = (struct rm_entity *)data;
	struct rm_entity *entity = container_of(t_l,struct rm_entity, callback_timer);

	printk(KERN_INFO "Call back for %d\n", entity->p->pid);


	if(entity->flags == 1){
		printk(KERN_INFO "Exceeded Deadline limit %d\n", entity->p->pid);
		rb_erase_cached(&(entity->rb_nd),&(runnables));

	}else{
		entity->flags = 1;
	}

	entity->deadline += entity->dl_deadline;

	rb_add_cached(&(entity->rb_nd), &runnables , __rm_less);

	__schedule_rm();

	mod_timer(&(entity->callback_timer), entity->deadline);


	//spin_unlock(&sched_lock);

}




int rm_dm_implementation( pid_t pid,unsigned int  period,unsigned int  deadline,unsigned int  exec_time){

	if (pid < 1)
		return -EINVAL;

	//spin_lock(&sched_lock);


	struct rm_entity *rm_of_task = kmalloc(sizeof(struct rm_entity), GFP_KERNEL);

	struct task_struct *task = find_get_task_by_vpid (pid);

	rm_of_task->p = task;
	rm_of_task->dl_runtime = exec_time;
	rm_of_task->dl_deadline = deadline;
	rm_of_task->dl_period = period;

	rm_of_task->flags = -1;


	rm_of_task->runtime = exec_time;
	rm_of_task->deadline = get_jiffies_64() + deadline;


	printk(KERN_INFO "The deadline of task %d is %llu \n",pid, rm_of_task->deadline );

//	rb_add_cached(&(rm_of_task->rb_nd), &runnables , __rm_less);

	list_add(&(rm_of_task->list_nd), &all_processes);



//	struct sched_param param = { .sched_priority = prio_of_process };
//
//	if (sched_setscheduler(task, SCHED_RR, &param) == -1) {
//		printk(KERN_INFO "sched_setscheduler Failed for %d\n", pid);
//		return -1;
//	}

	struct sched_attr *attr = kmalloc(sizeof(struct sched_attr), GFP_KERNEL);
	attr->size = sizeof(struct sched_attr);
	attr->sched_policy = SCHED_RR;
	attr->sched_priority = prio_of_process;


	if (sched_setattr(task, attr) == -1) {
		printk(KERN_ALERT "sched_setattr Failed for %d\n", pid);
//			return -1;
	}

	cpumask_t cpu_mask;
	cpumask_clear(&cpu_mask);
	cpumask_set_cpu(0, &cpu_mask);


	if (set_cpus_allowed_ptr(task, &cpu_mask) == -1) {
		printk(KERN_ALERT "set_cpus_allowed_ptr failed for PID %d\n", pid);
//		return -1;
	}

//	if (send_sig(SIGSTOP, task, 0) < 0) {
//		printk(KERN_ALERT "send_sig SIGSTOP failed for task %d\n", pid);
////		return -1;
//	}

	printk(KERN_INFO "Added the new process %d\n", task->pid);


	struct rm_entity *entity;
	printk(KERN_INFO "Printing process information from list...\n");

	list_for_each_entry(entity, &all_processes, list_nd) {
		printk(KERN_INFO "PID: %d, period: %llu, deadline: %llu, execution time: %lld\n",
		       entity->p->pid, entity->dl_period, entity->dl_deadline, entity->dl_runtime);
	}


	(rm_of_task->callback_timer).expires = rm_of_task->deadline;
	(rm_of_task->callback_timer).function = callback_deadline_setter;

	add_timer(&(rm_of_task->callback_timer));
//	timer_setup(&(rm_of_task->callback_timer), callback_deadline_setter, (unsigned long)&rm_of_task);
//
//	mod_timer(&(rm_of_task->callback_timer), jiffies + msecs_to_jiffies(deadline));

	__schedule_rm();

	//spin_unlock(&sched_lock);

	return 0;

}


bool check_for_rm_dm(pid_t pid, unsigned int deadline, unsigned int period, unsigned int exec_time){

	struct rm_entity *entity;
	struct task_struct *task;


	struct rm_entity* entity_for_interference;

	//spin_lock(&sched_lock);

	u64 t = 0;

	printk(KERN_INFO "Checking validity %d\n", pid);

	list_for_each_entry(entity, &all_processes, list_nd) {
		t += entity->dl_runtime;
		int cont = 1;

		while(cont){
			u64 interference = 0;
			list_for_each_entry(entity_for_interference, &all_processes, list_nd) {
				if (entity_for_interference->p->pid ==
				    entity->p->pid) {
					break;
				}

				u64 rem;

				interference +=
					div64_u64_rem(t,
						      entity_for_interference
							      ->dl_period,
						      &rem) *
					(entity_for_interference->dl_runtime);
				if (rem) {
					interference += (entity_for_interference
								 ->dl_runtime);
				}
			}

			if (interference + exec_time < t){
				cont = 0;
			}else{
				t = interference + exec_time;
			}


			if(t > deadline){
				return false;
			}
			
		}

	}

	t += exec_time;

	int cont = 1;

	while(cont){
		u64 interference = 0;
		list_for_each_entry(entity_for_interference, &all_processes, list_nd) {
			u64 rem;

			interference +=
				div64_u64_rem(
					t, entity_for_interference->dl_period,
					&rem) *
				(entity_for_interference->dl_runtime);
			if (rem) {
				interference +=
					(entity_for_interference->dl_runtime);
			}
		}

		if (interference + exec_time < t){
			cont = 0;
		}else{
			t = interference + exec_time;
		}


		if(t > entity-> deadline){
			return false;
		}

	}

	return true;
}



SYSCALL_DEFINE4(register_rm, pid_t, pid,unsigned int ,period,unsigned int ,deadline,unsigned int ,exec_time)
{

//	if(check_for_rm_dm(pid,deadline, period, exec_time)){
		return rm_dm_implementation(pid,period,deadline,exec_time);
//
//	}else{
//		return  -1;
//	}


}

SYSCALL_DEFINE4(register_dm, pid_t, pid,unsigned int ,period,unsigned int ,deadline,unsigned int ,exec_time)
{

	if (pid < 1)
		return -EINVAL;

//	if(check_for_rm_dm(pid,deadline, period, exec_time)){
		return rm_dm_implementation(pid,period,deadline,exec_time);
//	}else{
//		return -1;
//	}


}

SYSCALL_DEFINE1(yield, pid_t, pid)
{

	if (pid < 1)
		return -EINVAL;

	struct rm_entity *entity;
	struct task_struct *task;

	//spin_lock(&sched_lock);


	printk(KERN_INFO "Yielding process %d\n", pid);

	list_for_each_entry(entity, &all_processes, list_nd) {
		task = entity->p;
		if (task && task->pid == pid) {
			printk(KERN_INFO "Found rm_entity with pid %d\n", pid);

			if(entity->flags != -1){
				entity->flags = 0;
				rb_erase_cached(&(entity->rb_nd),&(runnables));
			}


			if (send_sig(SIGSTOP, task, 0) < 0) {
				printk(KERN_ALERT "send_sig SIGSTOP failed for task %d\n", pid);
//				return -1;
			}

			printk(KERN_INFO "Erased, stopped and set flags for %d\n", pid);

			break;
		}
	}


	__schedule_rm();

	//spin_unlock(&sched_lock);
	

	return 0;

}

SYSCALL_DEFINE1(remove, pid_t, pid)
{
	if (pid < 1)
		return -EINVAL;

	struct rm_entity *entity;
	struct task_struct *task;

	//spin_lock(&sched_lock);

	printk(KERN_INFO "Remove has been called for %d\n", pid);


	list_for_each_entry(entity, &all_processes, list_nd) {
		task = entity->p;
		if (task && task->pid == pid) {

			if(entity->flags == 1)
				rb_erase_cached(&(entity->rb_nd),&(runnables));

			list_del(&(entity->list_nd));

			entity->flags = 0;

			struct sched_param param;
			param.sched_priority = 0;


			if (sched_setscheduler(task, SCHED_NORMAL, &param) == -1) {
				printk(KERN_ALERT "SCHED_NORMAL problem with pid %d\n", pid);
//				return -1;
			}


			del_timer(&(entity->callback_timer));

			if(last_run_by_me->p->pid == pid){
				last_run_by_me = NULL;
			}

			printk(KERN_INFO "Removed, deleted and normalised %d\n", pid);


//			printk(KERN_INFO "Found rm_entity with pid %d\n", pid);
			break;
		}
	};

	//spin_unlock(&sched_lock);


	return 0;

}

SYSCALL_DEFINE0(list){

	//spin_lock(&sched_lock);

	struct rm_entity *entity;
	printk(KERN_INFO "Printing process information...\n");

	list_for_each_entry(entity, &all_processes, list_nd) {
		printk(KERN_INFO "PID: %d, period: %llu, deadline: %llu, execution time: %lld\n",
		       entity->p->pid, entity->dl_period, entity->dl_deadline, entity->dl_runtime);
	}

	//spin_unlock(&sched_lock);

	return 0;

}



// Define a struct for each resource
struct resource_pcp {
	unsigned int rid;

	u64 resource_ceil;

	struct list_head glob_list; // For list of all resources
	struct list_head my_list; // All the processes requesting this resource
};

// Define a struct for each process
struct process_pcp {
	pid_t pid;

	u64 process_prio;

	struct list_head glob_list; // For list of all processes
	struct list_head my_list; // All the resources requested by this process
};

LIST_HEAD(resource_list_pcp);

LIST_HEAD(process_list_pcp);



void __sched_pcp(void){
	
}


u64 global_ceil = 10000000;


SYSCALL_DEFINE2(resource_map, pid_t, pid, unsigned int, rid){


	struct process_pcp *process;
	struct resource_pcp *resource;
	int process_found = 0, resource_found = 0;

	// Check if the process already exists in the process list
	list_for_each_entry(process, &process_list_pcp, glob_list) {
		if (process->pid == pid) {
			process_found = 1;
			break;
		}
	}

	// If the process is not found, create a new process struct and add it to the list
	if (!process_found) {
		process = kmalloc(sizeof(struct process_pcp), GFP_KERNEL);
		if (!process) {
			printk(KERN_ERR "Error: Failed to allocate memory for process struct\n");
			return -ENOMEM;
		}
		process->pid = pid;
		INIT_LIST_HEAD(&process->my_list);
		list_add(&process->glob_list, &process_list_pcp);
	}

	// Check if the resource already exists in the resource list
	list_for_each_entry(resource, &resource_list_pcp, glob_list) {
		if (resource->rid == rid) {
			resource_found = 1;
			break;
		}
	}

	// If the resource is not found, create a new resource struct and add it to the list
	if (!resource_found) {
		resource = kmalloc(sizeof(struct resource_pcp), GFP_KERNEL);
		if (!resource) {
			printk(KERN_ERR "Error: Failed to allocate memory for resource struct\n");
			return -ENOMEM;
		}
		resource->rid = rid;
		resource->resource_ceil = 100000000;
		INIT_LIST_HEAD(&resource->my_list);
		list_add(&resource->glob_list, &resource_list_pcp);
	}


	struct process_pcp *process_to_add_to_res;
	struct resource_pcp *resource_to_add_to_process;


	process_to_add_to_res = kmalloc(sizeof(struct process_pcp), GFP_KERNEL);
	resource_to_add_to_process = kmalloc(sizeof(struct resource_pcp), GFP_KERNEL);

	process_to_add_to_res->pid = pid;


	list_for_each_entry(entity, &all_processes, list_nd) {
		if(entity->p->pid == pid){
			process_to_add_to_res->process_prio = entity->dl_deadline;
			break;
		}

	}

	resource_to_add_to_process->rid = rid;

	// Add the process to the resource's list of processes
	list_add(&process_to_add_to_res->my_list, &resource->my_list);

	// Add the resource to the process's list of resources
	list_add(&resource_to_add_to_process->my_list, &process->my_list);

	return 0;

}
SYSCALL_DEFINE1(start_pcp,unsigned int, RID){

	struct process_pcp *process;
	struct resource_pcp *resource;

	list_for_each_entry(resource, &resource_list_pcp, glob_list) {
		list_for_each_entry(process, &(resource->my_list), my_list) {
			resource->resource_ceil = min(resource->resource_ceil , process->process_prio);
			global_ceil = min(global_ceil, resource->resource_ceil);
		}
	}

	return  0;
}
SYSCALL_DEFINE2(pcp_lock, pid_t, pid, unsigned int, RID){
	return  0;
}
SYSCALL_DEFINE2(pcp_unlock, pid_t, pid, unsigned int, RID){
	return  0;
}








