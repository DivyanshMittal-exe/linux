//
// Created by higgsboson on 29/3/23.
//
*/
#include <linux/energy_model.h>
#include <linux/mmap_lock.h>
#include <linux/hugetlb_inline.h>
#include <linux/jiffies.h>
#include <linux/mm_api.h>
#include <linux/highmem.h>
#include <linux/spinlock_api.h>
#include <linux/cpumask_api.h>
#include <linux/lockdep_api.h>
#include <linux/softirq.h>
#include <linux/refcount_api.h>
#include <linux/topology.h>
#include <linux/sched/clock.h>
#include <linux/sched/cond_resched.h>
#include <linux/sched/cputime.h>
#include <linux/sched/isolation.h>
#include <linux/sched/nohz.h>

#include <linux/cpuidle.h>
#include <linux/interrupt.h>
#include <linux/memory-tiers.h>
#include <linux/mempolicy.h>
#include <linux/mutex_api.h>
#include <linux/profile.h>
#include <linux/psi.h>
#include <linux/ratelimit.h>
#include <linux/task_work.h>

#include <asm/switch_to.h>

#include <linux/sched/cond_resched.h>

#include "sched.h"
#include "stats.h"
#include "autogroup.h"

#include <linux/rbtree.h>
#include <linux/sched.h>



static inline int on_rm_rq(struct sched_rm_entity *rm_se)
{
	return !list_empty(&(rm_se->list_element));
}


static void enqueue_task_rm(struct rq *rq, struct task_struct *p, int flags){


	struct sched_rm_entity *rm_of_task= &(p->rm);
	struct rm_rq *rm_of_q = &(rq->rm);

	list_add(&(rm_of_task->list_element),&(rm_of_q->list_root));


}


static void dequeue_task_rm(struct rq *rq, struct task_struct *p, int flags){
	struct sched_rm_entity *rm_of_task= &(p->rm);
	struct rm_rq *rm_of_q = &(rq->rm);

	list_del(&(rm_of_task->list_element));

}

//static void update_curr_rm(struct rq *rq){
//
//	struct task_struct *curr = rq->curr;
//	struct sched_dl_entity *rm_se = &curr->rm;
//	u64 delta_exec, scaled_delta_exec;
//	int cpu = cpu_of(rq);
//	u64 now;
//
//	now = rq_clock_task(rq);
//	delta_exec = now - curr->se.exec_start;
//
//	schedstat_set(curr->stats.exec_max,
//		      max(curr->stats.exec_max, delta_exec));
//
//	trace_sched_stat_runtime(curr, delta_exec, 0);
//
//	update_current_exec_runtime(curr, now, delta_exec);
//
//
//	unsigned long scale_freq = arch_scale_freq_capacity(cpu);
//	unsigned long scale_cpu = arch_scale_cpu_capacity(cpu);
//
//	scaled_delta_exec = cap_scale(delta_exec, scale_freq);
//	scaled_delta_exec = cap_scale(scaled_delta_exec, scale_cpu);
//
//	rm_se->runtime -= scaled_delta_exec;
//
//
//}


//static void yield_task_rm(struct rq *rq)
//{
//	/*
//	 * We make the task go to sleep until its current deadline by
//	 * forcing its runtime to zero. This way, update_curr_dl() stops
//	 * it and the bandwidth timer will wake it up and will give it
//	 * new scheduling parameters (thanks to dl_yielded=1).
//	 */
//	rq->curr->rm.rm_yielded = 1;
//
//	update_rq_clock(rq);
//	update_curr_rm(rq);
//	/*
//	 * Tell update_rq_clock() that we've just updated,
//	 * so we don't do microscopic update in schedule()
//	 * and double the fastpath cost.
//	 */
//	rq_clock_skip_update(rq);
//}





static void set_next_task_rm(struct rq *rq, struct task_struct *p, bool first)
{
	struct sched_rm_entity *rm_of_task= &(p->rm);
	struct rm_rq *rm_of_q = &(rq->rm);

	p->se.exec_start = rq_clock_task(rq);



//	if (!list_empty(&(rm_of_task->list_element))){
//
//		struct sched_statistics *stats;
//
//		if (!schedstat_enabled())
//			return;
//
//	}
//		update_stats_wait_end_dl(dl_rq, dl_se);

//
//	/* You can't push away the running task */
//	dequeue_pushable_dl_task(rq, p);

	if (!first)
		return;

	// MP not needed as #defined only for arm
//	if (hrtick_enabled_dl(rq))
//		start_hrtick_dl(rq, p);


//	if (rq->curr->sched_class != &rm_sched_class)
//		update_dl_rq_load_avg(rq_clock_pelt(rq), rq, 0);

//	deadline_queue_push_tasks(rq);
}





static struct task_struct *pick_next_task_rm(struct rq *rq)
{
	struct sched_rm_entity *rm_entities;
	struct list_head *pos;

	struct sched_rm_entity  *min_entity = nullptr;
	u64 min_deadline = UINT64_MAX;

//	struct sched_rm_entity *rm_of_task= &(p->rm);
	struct rm_rq *rm_of_q = &(rq->rm);


	list_for_each(pos, &(rm_of_q->list_root)) {
		rm_entities = list_entry(pos, struct sched_rm_entity, list_element);

		if (ktime_get_ns() > rm_entities->deadline) {
			entry->deadline += rm_entities->dl_deadline;
			entry->runtime = rm_entities->dl_runtime;
		}

		if (rm_entities->runtime > 0 && rm_entities->deadline < min_deadline) {
			min_deadline = rm_entities->deadline;
			min_entity = rm_entities;
		}
	}

	if(min_entity == nullptr){
		return nullptr;
	}

	task_struct *to_run = container_of(min_entity, struct task_struct, rm);

	if (to_run){
		set_next_task_rm(rq, to_run, true);
	}

	return to_run;


}





static void update_curr_rm(struct rq *rq)
{

	struct task_struct *curr = rq->curr;
	struct sched_rm_entity *rm_se = &curr->rm;
	u64 delta_exec, scaled_delta_exec;
	int cpu = cpu_of(rq);
	u64 now;

	if (!on_rm_rq(rm_se))
		return;

	now = rq_clock_task(rq);
	delta_exec = now - curr->se.exec_start;

	update_current_exec_runtime(curr, now, delta_exec);

	unsigned long scale_freq = arch_scale_freq_capacity(cpu);
	unsigned long scale_cpu = arch_scale_cpu_capacity(cpu);

	scaled_delta_exec = cap_scale(delta_exec, scale_freq);
	scaled_delta_exec = cap_scale(scaled_delta_exec, scale_cpu);

	rm_se->runtime -= scaled_delta_exec;

}


static void put_prev_task_rm(struct rq *rq, struct task_struct *p)
{
	struct sched_rm_entity *rm_of_task= &(p->rm);
	struct rm_rq *rm_of_q = &(rq->rm);

//	if (on_dl_rq(&p->dl))
//		update_stats_wait_start_dl(dl_rq, dl_se);

	update_curr_rm(rq);

}



static void check_preempt_curr_rm(struct rq *rq, struct task_struct *p,
				  int flags)
{
		struct sched_rm_entity *a = &p->rm;
		struct sched_rm_entity *b = &rq->curr->rm;

		if(a->deadline < b-> deadline){
			resched_curr(rq);
			return ;
		}



}


static void yield_task_rm(struct rq *rq)
{


		update_rq_clock(rq);
		update_curr_rm(rq);

		rq_clock_skip_update(rq);
}


static void task_tick_rk(struct rq *rq, struct task_struct *p, int queued)
{
		update_curr_rm(rq);

//		update_dl_rq_load_avg(rq_clock_pelt(rq), rq, 1);
//		/*
//	 * Even when we have runtime, update_curr_dl() might have resulted in us
//	 * not being the leftmost task anymore. In that case NEED_RESCHED will
//	 * be set and schedule() will start a new hrtick for the next task.
//	 */
//		if (hrtick_enabled_dl(rq) && queued && p->dl.runtime > 0 &&
//		    is_leftmost(p, &rq->dl))
//			start_hrtick_dl(rq, p);
}

DEFINE_SCHED_CLASS(rm) = {

	.enqueue_task		= enqueue_task_rm,
	.dequeue_task		= dequeue_task_rm,
	.yield_task		= yield_task_rm,

	.check_preempt_curr	= check_preempt_curr_rm,

	.pick_next_task		= pick_next_task_rm,
	.put_prev_task		= put_prev_task_rm,
	.set_next_task          = set_next_task_rm,
//
//
//	#ifdef CONFIG_SMP
//		.balance		= balance_dl,
//		.pick_task		= pick_task_dl,
//		.select_task_rq		= select_task_rq_dl,
//		.migrate_task_rq	= migrate_task_rq_dl,
//		.set_cpus_allowed       = set_cpus_allowed_dl,
//		.rq_online              = rq_online_dl,
//		.rq_offline             = rq_offline_dl,
//		.task_woken		= task_woken_dl,
//		.find_lock_rq		= find_lock_later_rq,
//	#endif
//
//		.task_tick		= task_tick_dl,
//		.task_fork              = task_fork_dl,
//
//		.prio_changed           = prio_changed_dl,
//		.switched_from		= switched_from_dl,
//		.switched_to		= switched_to_dl,
//
		.update_curr		= update_curr_rm,
};