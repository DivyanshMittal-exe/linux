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

//struct rb_root_cached runnables = RB_ROOT_CACHED;

static LIST_HEAD(all_processes);

static DEFINE_SPINLOCK(sched_lock);


struct rm_entity *last_run_by_me = NULL;

#define  cpu_to_run_on  0;
#define prio_of_process  32;

#define MAX_GLOB_CEIL 1000000;

u64 global_ceil = MAX_GLOB_CEIL;


int pcp_begin = 0;

struct rm_entity{

//	struct rb_node rb_nd;

	struct list_head list_nd;

	struct task_struct *p;

	struct timer_list callback_timer;

	struct list_head wait_list;

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


//static  bool __rm_less(struct rb_node *a, const struct rb_node *b)
//{
//	struct rm_entity *f =   rb_entry((a), struct rm_entity, rb_nd);
//	struct rm_entity *s =   rb_entry((b), struct rm_entity, rb_nd);
//	return f->dl_priority < s->dl_priority;
//}






void __schedule_rm(void){

//	struct rm_entity* curr_entity = last_run_by_me;

	struct rm_entity* pos;
	struct rm_entity* n;


//	rbtree_postorder_for_each_entry_safe(pos, n, (&(runnables.rb_root)), rb_nd){
//		printk(KERN_INFO "RB Tree info %d Prio %d %c \n", pos->p->pid, pos->dl_priority, task_state_to_char(pos->p));
//	}

	printk(KERN_INFO "Inside Sched");


	struct rm_entity *entity ;

	list_for_each_entry(entity, &all_processes, list_nd) {
		printk(KERN_INFO "List info %d Prio %d %c Flags %d \n", entity->p->pid,
		       entity->dl_priority, task_state_to_char(entity->p) , entity->flags);
	}


	list_for_each_entry(entity, &all_processes, list_nd) {

		if(entity -> flags >= 0 && task_state_to_char(entity->p) == 'R'){
			if (send_sig(SIGSTOP, entity->p, 0) < 0) {

	//			curr_entity->flags = 0;
				printk(KERN_INFO "send_sig SIGSTOP failed for task %d\n", entity->p->pid);
				return;
			}

		}


	}

	printk(KERN_INFO "Stopped Processes");


	u64 min_prio = MAX_GLOB_CEIL;
	struct rm_entity *min_ent = NULL;

	list_for_each_entry(entity, &all_processes, list_nd) {
		if(entity->flags == 1){
			min_prio = min(min_prio, entity->dl_priority );
			if(entity->dl_priority == min_prio){
				min_ent = entity;
			}
		}
	}

	if(min_ent){
		if (send_sig(SIGCONT, min_ent->p, 0) < 0) {
			//			leftmost_rm_entity->flags = 1;
			printk(KERN_INFO "send_sig SIGCONT failed for task %d\n", min_ent->p->pid);
			return;
		}

	}

	int all_done = 1;

	list_for_each_entry(entity, &all_processes, list_nd) {
		if(entity ->flags != -2){
			all_done = 0;
			break;
		}
	}

	if(all_done){
		struct rm_entity *s, *tmp;
		list_for_each_entry_safe(s, tmp, &all_processes, list_nd) {
			list_del(&s->list_nd);
			kfree(s);
		}
	}

	printk(KERN_INFO "Done With Scheduling");


	return;

		//


		//	struct rm_entity* entity;
//	list_for_each_entry(entity, &all_processes, list_nd) {
//		printk(KERN_INFO "List info %d Prio %d %c \n", entity->p->pid, entity->dl_priority, task_state_to_char(entity->p));
//	}

//	if(RB_EMPTY_ROOT(&(runnables.rb_root))){
//		return ;
//	}

//	if(!curr_entity){
//		struct rb_node* leftmost;
//		struct rm_entity* leftmost_rm_entity;
//		leftmost = rb_first_cached(&runnables);
//		struct task_struct* leftmost_task_struct;
//		leftmost_rm_entity= container_of(leftmost, struct rm_entity, rb_nd);
//		leftmost_task_struct = leftmost_rm_entity->p;
//
//
//
//		char state_of_l_task = task_state_to_char(leftmost_task_struct);
//
//		printk(KERN_INFO "First Process added %d State %c\n", leftmost_task_struct->pid,state_of_l_task);
//
//		if (send_sig(SIGCONT, leftmost_task_struct, 0) < 0) {
//			printk(KERN_INFO "send_sig SIGCONT failed for task %d  First Process\n", leftmost_task_struct->pid);
//			return;
//		}
//
//		last_run_by_me = leftmost_rm_entity;
////		leftmost_rm_entity->flags = 1;
//		return ;
//	}


//	struct task_struct *curr_task = last_run_by_me->p;
//
//
//
//
//	struct rb_node* leftmost;
//	struct rm_entity* leftmost_rm_entity;
//	struct task_struct* leftmost_task_struct;
//	char state_of_curr_task;
//
//
//	leftmost = rb_first_cached(&runnables);
//	leftmost_rm_entity= container_of(leftmost, struct rm_entity, rb_nd);
//	leftmost_task_struct = leftmost_rm_entity->p;
//
//	state_of_curr_task = task_state_to_char(curr_task);
//
//
//	printk(KERN_INFO "Currently running %d, with deadline %llu, leftmost is %d, with deadline %llu\n", curr_task->pid, last_run_by_me->dl_deadline, leftmost_rm_entity->p->pid, leftmost_rm_entity->dl_deadline);
//
//
//	if(state_of_curr_task != 'R' || curr_entity->flags != 1 || curr_entity->dl_priority > leftmost_rm_entity->dl_priority){
//
//
//		if(state_of_curr_task == 'R'){
//			if (send_sig(SIGSTOP, curr_task, 0) < 0) {
//
//	//			curr_entity->flags = 0;
//
//				printk(KERN_INFO "send_sig SIGSTOP failed for task %d\n", curr_task->pid);
//				return;
//			}
//		}
//
//		if (send_sig(SIGCONT, leftmost_task_struct, 0) < 0) {
////			leftmost_rm_entity->flags = 1;
//			printk(KERN_INFO "send_sig SIGCONT failed for task %d\n", leftmost_task_struct->pid);
//			return;
//		}
//
//		last_run_by_me = leftmost_rm_entity;
//	}
//
//	printk(KERN_INFO "So I schedule %d, with deadline %lld\n", last_run_by_me->p->pid, last_run_by_me->dl_deadline);
//
//


}



void callback_deadline_setter(struct timer_list *t_l)
{

	spin_lock(&sched_lock);


	//	struct rm_entity *entity = (struct rm_entity *)data;
	struct rm_entity *entity = container_of(t_l,struct rm_entity, callback_timer);

//	printk(KERN_INFO "Call back for %d\n", entity->p->pid);
//
//	printk(KERN_INFO "Inside CB");
//
//	list_for_each_entry(entity, &all_processes, list_nd) {
//		printk(KERN_INFO "List info %d Prio %d %c \n", entity->p->pid, entity->dl_priority, task_state_to_char(entity->p));
//	}
//
//
//	if(entity->flags == 1){
//		printk(KERN_INFO "Exceeded Deadline limit %d\n", entity->p->pid);
//		rb_erase_cached(&(entity->rb_nd),&(runnables));
//
//	}else{
//		entity->flags = 1;
//	}


	entity->flags = 1;
	entity->deadline += msecs_to_jiffies(entity->dl_deadline);


//	struct rm_entity* pos;
//	struct rm_entity* n;
//
//	printk(KERN_INFO "RB Tree info Before Sched in Callback\n");
//
//	rbtree_postorder_for_each_entry_safe(pos, n, (&(runnables.rb_root)), rb_nd){
//		printk(KERN_INFO "RB Tree info %d Prio %d %c \n", pos->p->pid, pos->dl_priority, task_state_to_char(pos->p));
//
//	}
//
//	printk(KERN_INFO "RB Tree info Callink sched in callback\n");
//
//
//	rb_add_cached(&(entity->rb_nd), &runnables , __rm_less);

	__schedule_rm();

	mod_timer(&(entity->callback_timer), entity->deadline);


	spin_unlock(&sched_lock);

}




int rm_dm_implementation( pid_t pid,unsigned int  period,unsigned int  deadline,unsigned int  exec_time){

	if (pid < 1)
		return -EINVAL;

//	spin_lock(&sched_lock);


	struct rm_entity *rm_of_task = kmalloc(sizeof(struct rm_entity), GFP_KERNEL);

	struct task_struct *task = find_get_task_by_vpid (pid);

	rm_of_task->p = task;
	rm_of_task->dl_runtime = exec_time;
	rm_of_task->dl_deadline = deadline;
	rm_of_task->dl_priority = deadline;
	rm_of_task->dl_period = period;

	rm_of_task->flags = -1;


	rm_of_task->runtime = exec_time;
	rm_of_task->deadline = get_jiffies_64() + msecs_to_jiffies(deadline);


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

//	__schedule_rm();


	return 0;

}


bool check_for_rm(pid_t pid, unsigned int deadline, unsigned int period, unsigned int exec_time){
	struct rm_entity *entity;

	int mult = 1000;

	int util_times_mult = 0;

	u64 rem;

	list_for_each_entry(entity, &all_processes, list_nd) {
		util_times_mult +=  div64_u64_rem(entity->dl_runtime * mult,entity->dl_period,&rem);
	}

	util_times_mult +=  div64_u64_rem(exec_time * mult,period,&rem);

	printk(KERN_INFO "Utilisation is %d / %d\n", util_times_mult,mult);

	if(util_times_mult*100 < 69*mult){
		return true;
	}
	return false;

}


bool check_for_dm(pid_t pid, unsigned int deadline, unsigned int period, unsigned int exec_time){

	struct rm_entity *entity;
	struct task_struct *task;


	struct rm_entity* entity_for_interference;

	//spin_lock(&sched_lock);

	u64 t = 0;

	printk(KERN_INFO "Checking validity %d\n", pid);

	list_for_each_entry(entity, &all_processes, list_nd) {
		t += entity->dl_runtime;

		printk(KERN_INFO "t is %d\n", t);

		int cont = 1;

		while(cont){
			u64 interference = 0;
			list_for_each_entry(entity_for_interference, &all_processes, list_nd) {
				if (entity_for_interference->p->pid == entity->p->pid) {
					break;
				}

				u64 rem;

				interference += div64_u64_rem(t, entity_for_interference->dl_period, &rem) *(entity_for_interference->dl_runtime);
				if (rem) {
					interference += (entity_for_interference->dl_runtime);
				}
			}

			printk(KERN_INFO "interference is %d\n", interference);


			if (interference + entity->dl_runtime <= t){
				cont = 0;
			}else{
				t = interference + entity->dl_runtime;
			}


			if(t > entity->dl_deadline){
				printk(KERN_INFO "Check is false %d\n",pid);

				return false;
			}

		}

	}

	t += exec_time;

	int cont = 1;

	printk(KERN_INFO "t is %d\n", t);

	while(cont){
		u64 interference = 0;
		list_for_each_entry(entity_for_interference, &all_processes, list_nd) {

			u64 rem;

			interference += div64_u64_rem(t, entity_for_interference->dl_period, &rem) *(entity_for_interference->dl_runtime);
			if (rem) {
				interference += (entity_for_interference->dl_runtime);
			}
		}


		printk(KERN_INFO "interference is %d\n", interference);

		if (interference + exec_time <= t){
			cont = 0;
		}else{
			t = interference + exec_time;
		}



		if(t > deadline){
			printk(KERN_INFO "Check is False %d\n",pid);

			return false;
		}

	}

	printk(KERN_INFO "Check is true %d\n",pid);
	return true;
}



SYSCALL_DEFINE4(register_rm, pid_t, pid,unsigned int ,period,unsigned int ,deadline,unsigned int ,exec_time)
{

	if(check_for_rm(pid,deadline, period, exec_time)){
		return rm_dm_implementation(pid,period,deadline,exec_time);
//
	}else{
		return  -1;
	}


}

SYSCALL_DEFINE4(register_dm, pid_t, pid,unsigned int ,period,unsigned int ,deadline,unsigned int ,exec_time)
{

	if (pid < 1)
		return -EINVAL;

	if(check_for_dm(pid,deadline, period, exec_time)){
		return rm_dm_implementation(pid,period,deadline,exec_time);
	}else{
		return -1;
	}


}

SYSCALL_DEFINE1(yield, pid_t, pid)
{

	if (pid < 1)
		return -EINVAL;

	struct rm_entity *entity;
	struct task_struct *task;

	spin_lock(&sched_lock);


	printk(KERN_INFO "Yielding process %d\n", pid);

	list_for_each_entry(entity, &all_processes, list_nd) {
		task = entity->p;
		if (task && task->pid == pid) {
			printk(KERN_INFO "Found rm_entity with pid %d\n", pid);
			entity->flags = 0;
//			if(entity->flags != -1){
//				entity->flags = 0;
//				rb_erase_cached(&(entity->rb_nd),&(runnables));
//			}


//			if (send_sig(SIGSTOP, task, 0) < 0) {
//				printk(KERN_ALERT "send_sig SIGSTOP failed for task %d\n", pid);
////				return -1;
//			}

			printk(KERN_INFO "Erased, stopped and set flags for %d\n", pid);

			break;
		}
	}


	__schedule_rm();

	spin_unlock(&sched_lock);


	return 0;

}

SYSCALL_DEFINE1(remove, pid_t, pid)
{
	if (pid < 1)
		return -EINVAL;

	struct rm_entity *entity = NULL;
	struct task_struct *task;

	spin_lock(&sched_lock);

	printk(KERN_INFO "Remove has been called for %d\n", pid);

	list_for_each_entry(entity, &all_processes, list_nd) {
		printk(KERN_INFO "List info %d Prio %d %c Flg %d \n", entity->p->pid, entity->dl_priority, task_state_to_char(entity->p),entity->flags);
	}

	list_for_each_entry(entity, &all_processes, list_nd) {
		if (entity && entity->p->pid == pid) {

			task = entity->p;


			entity->flags = -2;

//			if(entity->flags == 1)
//				rb_erase_cached(&(entity->rb_nd),&(runnables));

//			list_del(&(entity->list_nd));

//			entity->flags = 0;

//			struct sched_param param;
//			param.sched_priority = 0;
//
//
//			if (sched_setscheduler(task, SCHED_NORMAL, &param) == -1) {
//				printk(KERN_ALERT "SCHED_NORMAL problem with pid %d\n", pid);
////				return -1;
//			}


			if (send_sig(SIGKILL, entity->p, 0) < 0) {
				//			leftmost_rm_entity->flags = 1;
				printk(KERN_INFO "send_sig SIGKILL failed for task %d\n", entity->p->pid);
				return -1;
			}

			del_timer(&(entity->callback_timer));


//			printk(KERN_INFO "Last run was %d\n", last_run_by_me->p->pid );

//			if(last_run_by_me->p->pid == pid){
//				last_run_by_me = NULL;
//			}

//			last_run_by_me = NULL;

			printk(KERN_INFO "Removed, deleted and normalised %d\n", pid);


//			printk(KERN_INFO "Found rm_entity with pid %d\n", pid);
			break;
		}
	}


	list_for_each_entry(entity, &all_processes, list_nd) {
		printk(KERN_INFO "List info %d Prio %d %c  Flag %d \n", entity->p->pid, entity->dl_priority, task_state_to_char(entity->p), entity->flags);
	}

	__schedule_rm();

	spin_unlock(&sched_lock);
//

	return 0;

}

SYSCALL_DEFINE0(list){

	spin_lock(&sched_lock);

	struct rm_entity *entity;
	printk(KERN_INFO "Printing process information...\n");

	list_for_each_entry(entity, &all_processes, list_nd) {
		printk(KERN_INFO "PID: %d, period: %llu, deadline: %llu, execution time: %lld\n",
		       entity->p->pid, entity->dl_period, entity->dl_deadline, entity->dl_runtime);
	}

	spin_unlock(&sched_lock);

	return 0;

}



// Define a struct for each resource
struct resource_pcp {
	unsigned int rid;

//	int acquired;

	u64 resource_ceil;

//	u64 resource_prio;

	pid_t acquires_pid; // -1 tells acquire status is false;


	struct list_head glob_list; // For list of all resources
	struct list_head wait_list; // All the processes requesting this resource
};

// Define a struct for each process
//struct process_pcp {
//	pid_t pid;
//
//	u64 process_prio;
//
//	struct list_head wait_list; //List of processes waiting for a resource
////	struct list_head my_list; // All the resources requested by this process
//};

LIST_HEAD(resource_list_pcp);

LIST_HEAD(process_list_pcp);



void __sched_pcp(void){

}



SYSCALL_DEFINE2(resource_map, pid_t, pid, unsigned int, rid){

	struct resource_pcp *resource;

	u64 proc_prio;

	struct rm_entity *entity;

	spin_lock(&sched_lock);


	list_for_each_entry(entity, &all_processes, list_nd) {
		if(entity->p->pid == pid){
			proc_prio = entity->dl_deadline;
			break;
		}
	}


	int resource_found = 0;

	list_for_each_entry(resource, &resource_list_pcp, glob_list) {
		if (resource->rid == rid) {
			resource_found = 1;
			break;
		}
	}

	if (!resource_found) {
		resource = kmalloc(sizeof(struct resource_pcp), GFP_KERNEL);
		if (!resource) {
			printk(KERN_ERR "Error: Failed to allocate memory for process struct\n");
			return -ENOMEM;
		}
		resource->rid = rid;
		resource->resource_ceil = MAX_GLOB_CEIL;
		resource->acquires_pid = -1;
//		resource->acquired = 0;
		INIT_LIST_HEAD(&resource->wait_list);
		list_add(&resource->glob_list, &resource_list_pcp);
	}

	resource->resource_ceil = min(resource->resource_ceil,proc_prio);

	struct resource_pcp *res_for_print;
	list_for_each_entry(res_for_print, &resource_list_pcp, glob_list) {

		printk(KERN_INFO "RID: %d Res Ceil: %d Res Acq: %d ",res_for_print->rid,res_for_print->resource_ceil,res_for_print->acquires_pid);

	}

	spin_unlock(&sched_lock);


		return 0;
//	resource->resource_prio = resource->resource_ceil;



//	struct process_pcp *process;
//	struct resource_pcp *resource;
//	int process_found = 0, resource_found = 0;
//
//	// Check if the process already exists in the process list
//	list_for_each_entry(process, &process_list_pcp, glob_list) {
//		if (process->pid == pid) {
//			process_found = 1;
//			break;
//		}
//	}
//
//	// If the process is not found, create a new process struct and add it to the list
//	if (!process_found) {
//		process = kmalloc(sizeof(struct process_pcp), GFP_KERNEL);
//		if (!process) {
//			printk(KERN_ERR "Error: Failed to allocate memory for process struct\n");
//			return -ENOMEM;
//		}
//		process->pid = pid;
//		INIT_LIST_HEAD(&process->my_list);
//		list_add(&process->glob_list, &process_list_pcp);
//	}
//
//	// Check if the resource already exists in the resource list
//	list_for_each_entry(resource, &resource_list_pcp, glob_list) {
//		if (resource->rid == rid) {
//			resource_found = 1;
//			break;
//		}
//	}
//
//	// If the resource is not found, create a new resource struct and add it to the list
//	if (!resource_found) {
//		resource = kmalloc(sizeof(struct resource_pcp), GFP_KERNEL);
//		if (!resource) {
//			printk(KERN_ERR "Error: Failed to allocate memory for resource struct\n");
//			return -ENOMEM;
//		}
//		resource->rid = rid;
//		resource->resource_ceil = 100000000;
//		INIT_LIST_HEAD(&resource->my_list);
//		list_add(&resource->glob_list, &resource_list_pcp);
//	}
//
//
//	struct process_pcp *process_to_add_to_res;
//	struct resource_pcp *resource_to_add_to_process;
//
//
//	process_to_add_to_res = kmalloc(sizeof(struct process_pcp), GFP_KERNEL);
//	resource_to_add_to_process = kmalloc(sizeof(struct resource_pcp), GFP_KERNEL);
//
//	process_to_add_to_res->pid = pid;
//
//
//	list_for_each_entry(entity, &all_processes, list_nd) {
//		if(entity->p->pid == pid){
//			process_to_add_to_res->process_prio = entity->dl_priority;
//			break;
//		}
//
//	}
//
//	resource_to_add_to_process->rid = rid;
//
//	// Add the process to the resource's list of processes
//	list_add(&process_to_add_to_res->my_list, &resource->my_list);
//
//	// Add the resource to the process's list of resources
//	list_add(&resource_to_add_to_process->my_list, &process->my_list);

//	return 0;

}


SYSCALL_DEFINE1(start_pcp,unsigned int, RID){

//	struct process_pcp *process;
//	struct resource_pcp *resource;
//
//	list_for_each_entry(resource, &resource_list_pcp, glob_list) {
//		list_for_each_entry(process, &(resource->my_list), my_list) {
//			resource->resource_ceil = min(resource->resource_ceil , process->process_prio);
//			global_ceil = min(global_ceil, resource->resource_ceil);
//		}
//	}


	pcp_begin = 1;

	return  0;
}









int pcp_lock_impl( pid_t pid, unsigned int RID)
{

	printk(KERN_INFO "Lock Called by PID: %d, RID: %d\n State of Resources: \n",pid,RID);

	spin_lock(&sched_lock);


	struct resource_pcp *res_for_print;
	list_for_each_entry(res_for_print, &resource_list_pcp, glob_list) {

		printk(KERN_INFO "Global Ceil: %d RID: %d Res Ceil: %d Res Acq: %d Wait List:: \n ",global_ceil,res_for_print->rid,res_for_print->resource_ceil,res_for_print->acquires_pid);

		struct rm_entity *entity_for_print;


		list_for_each_entry(entity_for_print, &(res_for_print->wait_list), wait_list) {
			printk(KERN_INFO "		Ent: %d, Prio: %d, Flag: %d \n", entity_for_print->p->pid, entity_for_print->dl_priority,entity_for_print->flags);
		}
	}


	u64 proc_prio;
	struct rm_entity *entity;
	struct rm_entity *this_entity;

	struct resource_pcp *resource;
	struct resource_pcp *resource_requested;

	list_for_each_entry(entity, &all_processes, list_nd) {
		if(entity->p->pid == pid){
			this_entity = entity;

			printk(KERN_INFO "PID: %d, RID: %d Got This Entity \n" ,pid,RID);

			break;
		}
	}

//	printk(KERN_INFO "PID: %d, RID: %d \n" ,pid,RID);
	printk(KERN_INFO "PID: %d, RID: %d Got Entity \n" ,pid,RID);

	int do_i_have_a_system_ceil = 0;

//	struct resource_pcp *resource;

	list_for_each_entry(resource, &resource_list_pcp, glob_list) {

		if(resource->acquires_pid == pid && resource->resource_ceil == global_ceil ){
			do_i_have_a_system_ceil = 1;
		}

		if (resource->rid == RID) {
			resource_requested = resource;
		}
	}

	printk(KERN_INFO "PID: %d, RID: %d Got Ceil and Resource \n" ,pid,RID);



	if(resource_requested->acquires_pid == -1){
		if(do_i_have_a_system_ceil != 0){
			resource_requested->acquires_pid = pid;

			printk(KERN_INFO "PID: %d, RID: %d I have the Ceil \n" ,pid,RID);

			spin_unlock(&sched_lock);

			return 0;
		}else if(this_entity->dl_priority < global_ceil){
			resource_requested->acquires_pid = pid;
			global_ceil = min(global_ceil,resource_requested->resource_ceil);

			printk(KERN_INFO "PID: %d, RID: %d I set the Ceil \n" ,pid,RID);

			spin_unlock(&sched_lock);

			return 0;
		}else{
//			rb_erase_cached(&(this_entity->rb_nd),&(runnables));
			list_add(&(this_entity->wait_list), &(resource_requested->wait_list));
			this_entity->flags = 3;

//			if (send_sig(SIGSTOP, this_entity->p, 0) < 0) {
//				printk(KERN_ALERT "send_sig SIGSTOP failed for task %d\n", pid);
//				//				return -1;
//			}

			printk(KERN_INFO "PID: %d, RID: %d I wait \n" ,pid,RID);


		}





	}else{

//		list_add(&(this_entity->wait_list), &(resource_requested->wait_list));

		struct rm_entity *entity_holding_the_resource;

		printk(KERN_INFO "PID: %d, RID: %d Somebody has this lock %d \n" ,pid,RID, resource_requested->acquires_pid);


		list_for_each_entry(entity_holding_the_resource, &all_processes, list_nd) {
			if(entity_holding_the_resource->p->pid == resource_requested->acquires_pid){
				printk(KERN_INFO "Found the holder \n");

				if(this_entity->dl_priority < entity_holding_the_resource-> dl_priority){
//					rb_erase_cached(&(entity_holding_the_resource->rb_nd),&(runnables));
					entity_holding_the_resource-> dl_priority = this_entity->dl_priority;
//					rb_add_cached(&(entity_holding_the_resource->rb_nd), &runnables , __rm_less);

				}
				break;
			}
		}

//		rb_erase_cached(&(this_entity->rb_nd),&(runnables));
		list_add(&(this_entity->wait_list), &(resource_requested->wait_list));

		this_entity->flags = 3;

//		if (send_sig(SIGSTOP, this_entity->p, 0) < 0) {
//			printk(KERN_ALERT "send_sig SIGSTOP failed for task %d\n", pid);
//			//				return -1;
//		}



	}

//	global_ceil = MAX_GLOB_CEIL;
//
//	list_for_each_entry(resource, &resource_list_pcp, glob_list) {
//
//		if(resource->acquires_pid != -1){
//			global_ceil = min(global_ceil, resource->resource_ceil);
//		}
//	}




	printk(KERN_INFO "Done with Lock");

	list_for_each_entry(res_for_print, &resource_list_pcp, glob_list) {

		printk(KERN_INFO "Global Ceil: %d RID: %d Res Ceil: %d Res Acq: %d Wait List:: \n ",global_ceil,res_for_print->rid,res_for_print->resource_ceil,res_for_print->acquires_pid);

		struct rm_entity *entity_for_print;


		list_for_each_entry(entity_for_print, &(res_for_print->wait_list), wait_list) {
			printk(KERN_INFO "		Ent: %d, Prio: %d, Flag: %d \n", entity_for_print->p->pid, entity_for_print->dl_priority,entity_for_print->flags);
		}
	}

	__schedule_rm();


	spin_unlock(&sched_lock);


	return  0;
}






SYSCALL_DEFINE2(pcp_lock, pid_t, pid, unsigned int, RID)
{
		return pcp_lock_impl(pid,RID);
}





SYSCALL_DEFINE2(pcp_unlock, pid_t, pid, unsigned int, RID){

	printk(KERN_INFO "Called Unlock %d on %d", pid, RID);

	struct resource_pcp *res_for_print;

	spin_lock(&sched_lock);


	list_for_each_entry(res_for_print, &resource_list_pcp, glob_list) {

		printk(KERN_INFO "Global Ceil: %d RID: %d Res Ceil: %d Res Acq: %d Wait List:: \n ",global_ceil,res_for_print->rid,res_for_print->resource_ceil,res_for_print->acquires_pid);

		struct rm_entity *entity_for_print;


		list_for_each_entry(entity_for_print, &(res_for_print->wait_list), wait_list) {
			printk(KERN_INFO "		Ent: %d, Prio: %d, Flag: %d \n", entity_for_print->p->pid, entity_for_print->dl_priority,entity_for_print->flags);
		}
	}

	global_ceil =MAX_GLOB_CEIL;

	struct rm_entity *entity;
	struct resource_pcp *resource;


	list_for_each_entry(entity, &all_processes, list_nd) {
		if(entity->p->pid == pid){
			entity->dl_priority = entity->dl_deadline;
			break;
		}
	}


	u64 min_prio_of_waiting_on_this_res;

	struct rm_entity * min_prio_wait_on_res_rm_ent;



	list_for_each_entry(resource, &resource_list_pcp, glob_list) {

		if(resource->rid == RID){
			resource->acquires_pid = -1;

		}


		if(resource->acquires_pid !=-1){
			global_ceil = min(global_ceil, resource-> resource_ceil);
		}


		if(resource->acquires_pid == pid ){
			entity->dl_priority = min(entity->dl_priority, resource-> resource_ceil);

			struct rm_entity *entities_waiting_for_resource;


			list_for_each_entry(entities_waiting_for_resource, &(resource->wait_list), wait_list) {

				entity->dl_priority = min(entity->dl_priority, entities_waiting_for_resource-> dl_priority);

			}

		}

	}

//	rb_erase_cached(&(entity->rb_nd),&(runnables));


//	rb_add_cached(&(entity->rb_nd), &runnables , __rm_less);

	printk(KERN_INFO "PID: %d, RID: %d Global Ceiling and Process Priority set \n" ,pid,RID);

	list_for_each_entry(res_for_print, &resource_list_pcp, glob_list) {

		printk(KERN_INFO "Global Ceil: %d RID: %d Res Ceil: %d Res Acq: %d Wait List:: \n ",global_ceil,res_for_print->rid,res_for_print->resource_ceil,res_for_print->acquires_pid);

		struct rm_entity *entity_for_print;


		list_for_each_entry(entity_for_print, &(res_for_print->wait_list), wait_list) {
			printk(KERN_INFO "		Ent: %d, Prio: %d, Flag: %d \n", entity_for_print->p->pid, entity_for_print->dl_priority,entity_for_print->flags);
		}
	}





	list_for_each_entry(resource, &resource_list_pcp, glob_list) {

		if(resource -> acquires_pid == -1){


			struct rm_entity *entity;
			struct rm_entity *min_entity_waiting = NULL;


			u64 min_entity_waiting_prio = MAX_GLOB_CEIL;

			list_for_each_entry(entity, &(resource->wait_list), wait_list) {

				min_entity_waiting_prio = min(min_entity_waiting_prio, entity->dl_priority);
				if(entity->dl_priority == min_entity_waiting_prio){
					min_entity_waiting = entity;
				}

			}

			if(min_entity_waiting){


				struct resource_pcp *resource_to_check_ceil;

				int am_i_system_ceil = 0;

				u64 min_entity_waiting_holding_system_ceil = min_entity_waiting->dl_priority;

				list_for_each_entry(resource_to_check_ceil, &resource_list_pcp, glob_list) {

					if(resource_to_check_ceil->acquires_pid == min_entity_waiting->p->pid &&  resource_to_check_ceil->resource_ceil == global_ceil){

						am_i_system_ceil = 1;
					}
				}

				if(am_i_system_ceil ){

					resource->acquires_pid = min_entity_waiting->p->pid;

					list_del(&(min_entity_waiting->wait_list));

					min_entity_waiting->flags = 1;

//					rb_add_cached(&(min_entity_waiting->rb_nd), &runnables , __rm_less);


				}else if(min_entity_waiting->dl_priority < global_ceil){
					resource->acquires_pid = min_entity_waiting->p->pid;
					global_ceil = min(global_ceil,resource->resource_ceil);

					list_del(&(min_entity_waiting->wait_list));

					min_entity_waiting->flags = 1;
//					rb_add_cached(&(min_entity_waiting->rb_nd), &runnables , __rm_less);
				}
			}

		}


	}

	printk(KERN_INFO "PID: %d, RID: %d Given Resources \n" ,pid,RID);




	printk(KERN_INFO "Done with Unlock");
	list_for_each_entry(res_for_print, &resource_list_pcp, glob_list) {

		printk(KERN_INFO "Global Ceil: %d RID: %d Res Ceil: %d Res Acq: %d Wait List:: \n ",global_ceil,res_for_print->rid,res_for_print->resource_ceil,res_for_print->acquires_pid);

		struct rm_entity *entity_for_print;


		list_for_each_entry(entity_for_print, &(res_for_print->wait_list), wait_list) {
			printk(KERN_INFO "		Ent: %d, Prio: %d, Flag: %d \n", entity_for_print->p->pid, entity_for_print->dl_priority,entity_for_print->flags);
		}
	}

	__schedule_rm();

	 spin_unlock(&sched_lock);


	return  0;
}








