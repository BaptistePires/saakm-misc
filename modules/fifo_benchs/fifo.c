#include "linux/lockdep.h"
#include "linux/sched.h"
#include "linux/smp.h"
#include <linux/module.h>
#include <linux/ipanema.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/proc_fs.h>
/*
 * This scheduler is a very simple FIFO scheduler without any load-balancing yet.
 * There is one runqueue per core, where a process is spawned (whether it was 
 * forked or switched sched class), it searches for the cpu with the least
 * tasks enqueued and picks it.
 */

MODULE_AUTHOR("Baptiste Pires, LIP6");
MODULE_LICENSE("GPL");

// #define FIFO_DEBUG 1

#define QUANTUM_MS 20
#define QUANTUM_NS 20 * 1000000
#define CORE_HAVE_CURRENT(c) ((c)->_current ? 1 : 0)

// extern atomic64_t __nr_ipanema_running;
struct ipanema_policy *fifo_policy;
static const char *policy_name = "fifo\0";
/*
    Represents a process.
*/
struct fifo_process {
	int state; /* IPANEMA state of the process */
	struct task_struct *task; /* Pointer to the task_struct of this process */
	struct ipanema_rq *rq; /* IPANEMA runqueue if it's enqueued in one */
	unsigned long current_quantum_ns; /* Total execution time*/
	unsigned long last_sched_ns; /* Last time scheduled timestamp */
};

/*
    Represents a core.
*/
struct fifo_core {
	enum ipanema_core_state state; /* Core state */
	int id; /* core id (== cpu id) */

	struct fifo_process
		*_current; /* the current process executing on this core */

	struct ipanema_rq rq; /* The runqueue allocated to this core. */

	struct {
		unsigned long total_enqueue;
		unsigned long total_exec;
		unsigned long schedule_calls;
		unsigned long tick_calls;	
		}	stats;
};

DEFINE_PER_CPU(struct fifo_core, core);

/*
    A cpumask that will be used to store which core are online or not (idle or deactivated).
*/
static cpumask_t online_cores;

/* ------------------------ CPU cores related functions ------------------------ */
static enum ipanema_core_state
fifo_get_core_state(struct ipanema_policy *policy, struct core_event *e)
{
	int target = e->target;
	return per_cpu(core, target).state;
}

/*
 * Core is reactivated
 */
static void fifo_core_entry(struct ipanema_policy *policy, struct core_event *e)
{
	struct fifo_core *c = &per_cpu(core, e->target);
	c->state = IPANEMA_ACTIVE_CORE;
	cpumask_set_cpu(c->id, &online_cores);
}

/*
 * Core is deactivated.
 */
static void fifo_core_exit(struct ipanema_policy *policy, struct core_event *e)
{
	struct fifo_core *c = &per_cpu(core, e->target);
	c->state = IPANEMA_IDLE_CORE;
	cpumask_clear_cpu(c->id, &online_cores);
}

/*
 * We're about to become idle, it can be a good place to steal tasks from busy cores.
 */
static void fifo_newly_idle(struct ipanema_policy *policy, struct core_event *e)
{
}

/*
 * We become idle for real.
 */
static void fifo_enter_idle(struct ipanema_policy *policy, struct core_event *e)
{
	per_cpu(core, e->target).state = IPANEMA_IDLE_CORE;
}

/*
 * We're leaving the idle state. Can happens when we were idle and a task picks
 * this core to be enqueued on.
 */
static void fifo_exit_idle(struct ipanema_policy *policy, struct core_event *e)
{
	per_cpu(core, e->target).state = IPANEMA_ACTIVE_CORE;
}

static void fifo_balancing_select(struct ipanema_policy *policy,
				  struct core_event *e)
{
}

/* ------------------------ CPU cores related functions END ------------------------ */

/* ------------------------ Processes related functions ------------------------ */

static int select_cpu(struct task_struct *p, int prev_cpu, int wake_flags)
{
	/*if (wake_flags & IPANEMA_WF_EXEC)*/
	/*	return prev_cpu;*/

	return p->pid % num_online_cpus();
}
/*
 * Helper function used to enqueue a task @fp when state is IPANEMA_READY
 * or IPANEMA_READY_TICK.
 */
static void fifo_enqueue_task(struct fifo_process *fp,
			      struct ipanema_rq *next_rq, int state)
{
	/* The current core on which the process is or will be */
	struct fifo_core *fc = &per_cpu(core, task_cpu(fp->task));

	/*
        * When switching from IPANEMA_RURNNING to IPANEMA_READY_TICK,
        * the process was removed from the cpu and we need to update it
        * for our core.
        */
	if (state == IPANEMA_READY_TICK) {
		fc->_current = NULL;
	}

	fc->stats.total_enqueue++;
	fp->state = state;
	fp->rq = next_rq;

	change_state(fp->task, state, task_cpu(fp->task), next_rq);
}

/*
    This function is called in "ipanema_new_prepare".
    We can be called in the following scenario :
        1. We're switching class
        2. In "select_task_rq_ipanema" when we're forked.

    The goal of this function is to initialize the 
    entity that will be used by this scheduler to schedule 
    this task and to find the cpu on which it should be 
    enqueued.
*/
static int fifo_new_prepare(struct ipanema_policy *policy,
			    struct process_event *e)
{
	struct fifo_process *fp;
	struct task_struct *p = e->target;

	fp = kzalloc(sizeof(struct fifo_process), GFP_ATOMIC);
	if (!fp)
		return -1;

	/* Init process structure */
	fp->task = p;
	fp->rq = NULL;
	p->ipanema.policy_metadata = fp;
	/*pr_info("new task : pid:%d comm:%s\n", p->pid, p->comm);*/
	//
	/*return select_cpu(p, task_cpu(p), e->flags);*/
	return select_cpu(p, task_cpu(p), 0);
}

/*
    This function is only called when a task is in a 
    IPANEMA_NOT_QUEUED state, right after "ipanema_new_prepare".
    
    We chosed a core to execute the task on before in "ipanema_new_prepare", 
    now do the actual enqueuing.
*/
static void fifo_new_place(struct ipanema_policy *policy,
			   struct process_event *e)
{
	struct fifo_process *fp = policy_metadata(e->target);
	int cpu_dst = task_cpu(fp->task);

	fifo_enqueue_task(fp, &per_cpu(core, cpu_dst).rq, IPANEMA_READY);
}

/*
    Seems to not be used yet.
*/
static void fifo_new_end(struct ipanema_policy *policy, struct process_event *e)
{
}

static void fifo_tick(struct ipanema_policy *policy, struct process_event *e)
{
	struct fifo_process *fp = policy_metadata(e->target);

	u64 now = ktime_get_ns();
	fp->current_quantum_ns += now - fp->last_sched_ns;
	
	/* Do switch there */
	if (fp->current_quantum_ns >= QUANTUM_NS) {
		fp->current_quantum_ns = 0;
		fp->last_sched_ns = 0;
		/* Tasks has used its whole quantum, make it READY instead of RUNNING. */
		fifo_enqueue_task(fp, &per_cpu(core, task_cpu(fp->task)).rq,
				  IPANEMA_READY_TICK);
	}

	per_cpu(core, task_cpu(fp->task)).stats.tick_calls++;
}

static void fifo_yield(struct ipanema_policy *policy, struct process_event *e)
{
	struct fifo_process *fp = policy_metadata(e->target);

	fifo_enqueue_task(fp, &per_cpu(core, task_cpu(fp->task)).rq, IPANEMA_READY);
}

static void fifo_block(struct ipanema_policy *policy, struct process_event *e)
{
	struct fifo_process *fp = policy_metadata(e->target);
	struct fifo_core *fc = &per_cpu(core, task_cpu(fp->task));

	fp->state = IPANEMA_BLOCKED;
	fp->rq = NULL;
	fc->_current = NULL;

	change_state(fp->task, IPANEMA_BLOCKED, task_cpu(fp->task), NULL);
}

/*
    The next two functions (fifo_unblock_prepare and fifo_unblock_place) follow the 
    same design as "new_prepare" and "new_place". The "prepare" step is to find a 
    cpu on which we can queue the task and place for the actual queuing.
*/
static int fifo_unblock_prepare(struct ipanema_policy *policy,
				struct process_event *e)
{
	/*return select_cpu(e->target, task_cpu(e->target), e->flags);*/
	return select_cpu(e->target, task_cpu(e->target), 0);
}

static void fifo_unblock_place(struct ipanema_policy *policy,
			       struct process_event *e)
{
	struct fifo_process *fp = policy_metadata(e->target);
	int cpu_dst = task_cpu(fp->task);

	fifo_enqueue_task(fp, &per_cpu(core, cpu_dst).rq, IPANEMA_READY);
}

static void fifo_unblock_end(struct ipanema_policy *policy,
			     struct process_event *e)
{
}

/*
    This functionis only called in "ipanema_terminate".
    We can be called in those scenario :
        1. We're switching out from ipanema
        2. The task is dying, therefore we need to remove it.
*/
static void fifo_terminate(struct ipanema_policy *policy,
			   struct process_event *e)
{
	struct fifo_process *fp = policy_metadata(e->target);
	struct fifo_core *fc = &per_cpu(core, task_cpu(fp->task));

	if (fp->state == IPANEMA_RUNNING) {
		fc->_current = NULL;
	}

	fp->state = IPANEMA_TERMINATED;
	fp->rq = NULL;

	change_state(fp->task, IPANEMA_TERMINATED, task_cpu(fp->task), NULL);

	kfree(fp);

	/*pr_info("Task terminated pid=%d comm=%s state=%d\n", fp->task->pid, fp->task->comm, fp->task->__state);*/
}

static void fifo_schedule(struct ipanema_policy *policy, unsigned int cpu)
{
	struct task_struct *next;
	struct fifo_process *fp, *fc_curr;
	struct task_struct *curr;
	struct fifo_core *fc = &per_cpu(core, cpu);

	curr = get_ipanema_current(cpu);

	if (curr) {
		fc_curr = policy_metadata(curr);
		fc->stats.total_exec += fc_curr->current_quantum_ns;
	}

	next = ipanema_first_task(&per_cpu(core, cpu).rq);

	if (!next) {
		return;
	}

	fp = policy_metadata(next);
	fp->state = IPANEMA_RUNNING;
	fp->rq = NULL;
	fp->current_quantum_ns = 0;
	fp->last_sched_ns = ktime_get_ns();

	fc->_current = fp;
	fc->stats.total_exec += fp->current_quantum_ns;
	fc->stats.schedule_calls++;
	change_state(fp->task, IPANEMA_RUNNING, task_cpu(fp->task), NULL);
}

static void fifo_load_balance(struct ipanema_policy *policy,
			      struct core_event *e)
{
}

/* ------------------------ Processes related functions END ------------------------ */

/* ------------------------ Module related functions ------------------------ */
static int fifo_init(struct ipanema_policy *policy)
{
	return 0;
}

static int fifo_free_metadata(struct ipanema_policy *policy)
{
	if (policy->data)
		kfree(policy->data);
	return 0;
}

static int fifo_can_be_default(struct ipanema_policy *policy)
{
	return 1;
}

static bool fifo_attach(struct ipanema_policy *policy, struct task_struct *task,
			char *command)
{
	return true;
}

/* ------------------------ Module related functions END ------------------------ */

struct ipanema_module_routines fifo_routines = {
	/* Processes related functions */
	.new_prepare = fifo_new_prepare,
	.new_end = fifo_new_end,
	.terminate = fifo_terminate,
	.new_place = fifo_new_place,
	.tick = fifo_tick,
	.yield = fifo_yield,
	.block = fifo_block,
	.unblock_prepare = fifo_unblock_prepare,
	.unblock_place = fifo_unblock_place,
	.unblock_end = fifo_unblock_end,
	.terminate = fifo_terminate,
	.schedule = fifo_schedule,
	.balancing_select = fifo_balancing_select,

	/* CPU cores functions */
	.get_core_state = fifo_get_core_state,
	.core_entry = fifo_core_entry,
	.core_exit = fifo_core_exit,
	.newly_idle = fifo_newly_idle,
	.enter_idle = fifo_enter_idle,
	.exit_idle = fifo_exit_idle,
	.balancing_select = fifo_load_balance,

	/* Policy related functions */
	.init = fifo_init,
	.free_metadata = fifo_free_metadata,
	.can_be_default = fifo_can_be_default,
	.attach = fifo_attach

};

/*
    lip6 sorbonne univeristé CNRS
    Debug FS functions.
*/
static int fifo_full_debug_info(struct seq_file *m, void *p)
{
	int cpu;
	unsigned int i = 0;
	struct fifo_core *fc;
	struct ipanema_rq *rq;
	struct task_struct *tsk;
	struct fifo_process *fp;
	// smp_load_acquire(&__nr_ipanema_running);
	// long nr_ipanema_running = atomic64_read(&__nr_ipanema_running);

	for_each_possible_cpu(cpu) {
		ipanema_lock_core(cpu);
		fc = &per_cpu(core, cpu);
		rq = &fc->rq;
		i = 0;

		seq_printf(m, "cpu :%d\n", cpu);
		seq_printf(m, "---------------------\n");
		seq_printf(m, "fifo_core struct\n");
		seq_printf(m, "---------------------\n");
		seq_printf(m, "\t _current : %p\n", fc->_current);
		seq_printf(m, "\t state    : %d\n", fc->state);
		seq_printf(m, "---------------------\n");
		seq_printf(m, "ipanema_rq : \n");
		seq_printf(m, "---------------------\n");
		seq_printf(m, "\t nr_tasks : %x\n", rq->nr_tasks);
		seq_printf(m, "\t state : %x\n", rq->state);
		seq_printf(m, "\t nr_enqueue : %lu\n", fc->stats.total_enqueue);
		seq_printf(m, "\t total_exec : %lu\n", fc->stats.total_exec);
		seq_printf(m, "\t schedule_calls : %lu\n",
			   fc->stats.schedule_calls);
		seq_printf(m, "\t tick_calls : %lu\n", fc->stats.tick_calls);
		list_for_each_entry(tsk, &rq->head, ipanema.node_list) {
			fp = policy_metadata(tsk);
			seq_printf(
				m,
				"\t\t #%u : pid=%d comm=%s state=%x ipanema_state=%x quantum=%lu\n",
				i, tsk->pid, tsk->comm, tsk->__state, fp->state,
				fp->current_quantum_ns);
			++i;
		}
		ipanema_unlock_core(cpu);
	}

	// seq_printf(m, "__nr_ipanema_running : %ld\n", nr_ipanema_running);

	return 0;
}

struct proc_dir_entry *proc_entry;

static int __init fifo_mod_init(void)
{
	int res;
	int cpu;
	struct fifo_core *c;
	/* Init cores */
	for_each_possible_cpu(cpu) {
		c = &per_cpu(core, cpu);
		c->id = cpu;
		c->state = IPANEMA_ACTIVE_CORE;
		/* No need for an order function as we're  FIFO. */
		init_ipanema_rq(&c->rq, FIFO, cpu, IPANEMA_READY, NULL);
		cpumask_set_cpu(cpu, &online_cores);

		c->stats.total_enqueue = 0;
		c->stats.total_exec = 0;
		c->stats.schedule_calls = 0;
		c->stats.tick_calls = 0;
	}

	fifo_policy = kzalloc(sizeof(struct ipanema_policy), GFP_KERNEL);
	if (!fifo_policy)
		return -ENOMEM;

	fifo_policy->kmodule = THIS_MODULE;
	fifo_policy->routines = &fifo_routines;
	strncpy(fifo_policy->name, policy_name, MAX_POLICY_NAME_LEN);

	res = ipanema_add_policy(fifo_policy);

	if (res) {
		kfree(fifo_policy);
	}

	proc_entry =
		proc_create_single("fifo_debug", 0, NULL, fifo_full_debug_info);
	return res;
}
module_init(fifo_mod_init);

static void __exit fifo_exit(void)
{
	int res;

	res = ipanema_remove_policy(fifo_policy);

	kfree(fifo_policy);

	printk(KERN_INFO "FIFO module unloaded\n");
	proc_remove(proc_entry);
}
module_exit(fifo_exit);
