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


	//struct rb_root_cached rb_nd_root = RB_ROOT_CACHED;


	//static struct task_struct *__pick_next_task_rm(struct rq *rq)
	//{
	//	struct *rm_rq = &rq->rm;
	////	struct rb_root_cached = rm_rq->root;
	//
	//	struct task_struct *p;
	//
	//
	//	struct rb_node *left = rb_first_cached(&rm_rq->root);
	//
	//	if (!left)
	//		return NULL;
	//
	//	struct sched_rm_entity *rm_rb_n = container_of(left, struct sched_rm_entity, rb_nd);
	//
	//	p = rm_rb_n->p;
	//
	//	return  p
	//
	////	return pick_next_task_fair(rq, NULL, NULL);
	//}

	//static inline bool __rm_less(struct rb_node *a, const struct rb_node *b)
	//{
	//	stuct sched_rm_entity *f =   rb_entry((a), struct sched_rm_entity, rb_nd);
	//	stuct sched_rm_entity *s =   rb_entry((b), struct sched_rm_entity, rb_nd);
	//	return f->rm_deadline < s->rm_deadline;
	//}

	//static void enqueue_task_rm(struct rq *rq, struct task_struct *p, int flags){
	//
	//
	//	struct sched_rm_entity *rm_ee= &(p->rm_e);
	//	struct rm_rq *rm_rq_e = &(rq->rm);
	//	rb_add_cached(&(rm_ee->rb_nd), &(rm_rq_e -> root), __rm_less);
	//
	//}


	static void enqueue_task_rm(struct rq *rq, struct task_struct *p, int flags){


	struct sched_rm_entity *rm_ee= &(p->rm_e);
	struct rm_rq *rm_rq_e = &(rq->rm);

	list_add(&(rm_ee->list_element),&(rm_rq_e->list_root));

	//	rb_add_cached(&(rm_ee->rb_nd), &(rm_rq_e -> root), __rm_less);

}


static void dequeue_task_rm(struct rq *rq, struct task_struct *p, int flags){
	struct sched_rm_entity *rm_ee= &(p->rm_e);
	struct rm_rq *rm_rq_e = &(rq->rm);

	list_del(&(rm_ee->list_element));
	//	rb_erase_cached(&(rm_ee->rb_nd), &(rm_rq_e -> root));


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


static void yield_task_rm(struct rq *rq)
{
	/*
	 * We make the task go to sleep until its current deadline by
	 * forcing its runtime to zero. This way, update_curr_dl() stops
	 * it and the bandwidth timer will wake it up and will give it
	 * new scheduling parameters (thanks to dl_yielded=1).
	 */
	rq->curr->rm.rm_yielded = 1;

	update_rq_clock(rq);
	update_curr_rm(rq);
	/*
	 * Tell update_rq_clock() that we've just updated,
	 * so we don't do microscopic update in schedule()
	 * and double the fastpath cost.
	 */
	rq_clock_skip_update(rq);
}


//static void check_preempt_curr_rm(struct rq *rq, struct task_struct *p,
//				  int flags){
//
//}

static struct task_struct *__pick_next_task_rm(struct rq *rq)
{
	struct task_struct *p;

	p = pick_task_dl(rq);
	if (p)
		set_next_task_dl(rq, p, true);

	return p;
}


DEFINE_SCHED_CLASS(rm) = {

	.enqueue_task		= enqueue_task_rm,
	.dequeue_task		= dequeue_task_rm,
	.yield_task		= yield_task_rm,

	//	.check_preempt_curr	= check_preempt_rm,

	.pick_next_task		= __pick_next_task_rm,
	.put_prev_task		= put_prev_task_fair,
	.set_next_task          = set_next_task_fair,

#ifdef CONFIG_SMP
	.balance		= balance_fair,
	.pick_task		= pick_task_fair,
	.select_task_rq		= select_task_rq_fair,
	.migrate_task_rq	= migrate_task_rq_fair,

	.rq_online		= rq_online_fair,
	.rq_offline		= rq_offline_fair,

	.task_dead		= task_dead_fair,
	.set_cpus_allowed	= set_cpus_allowed_common,
#endif

	.task_tick		= task_tick_fair,
	.task_fork		= task_fork_fair,

	.prio_changed		= prio_changed_fair,
	.switched_from		= switched_from_fair,
	.switched_to		= switched_to_fair,

	.get_rr_interval	= get_rr_interval_fair,

	.update_curr		= update_curr_rm,

#ifdef CONFIG_FAIR_GROUP_SCHED
	.task_change_group	= task_change_group_fair,
#endif

#ifdef CONFIG_UCLAMP_TASK
	.uclamp_enabled		= 1,
#endif
};