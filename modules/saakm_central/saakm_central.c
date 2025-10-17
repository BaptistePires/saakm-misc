#include "linux/cpumask.h"
#include "linux/cpumask_types.h"
#include "linux/hrtimer_types.h"
#include "linux/jiffies.h"
#include "linux/ktime.h"
#include "linux/sched.h"
#include "linux/smp.h"
#include "linux/time.h"
#include "linux/workqueue.h"
#include <linux/module.h>
#include <linux/saakm.h>

/*
 * This is an implementation of the scx_central scheduling policy from scx. It is based on Linux v6.12.
 * This policy uses a central cpu that take scheduling decisions for all other cpus.
 * It currently supports : 
 *      - Same idle masks handling 
 *

 * Pour le moment, la runqueue du CPU 0 est la runqueue centrale. Quand il doit envoyer une tache sur un autre cpu il prend la premiere de sa rq et l'envoie la bas. Quand il doit prendre une tâche pour lui même, il prend la premiere de sa rq.

 * TODO : 
        - Check ENQ_LAST flag
*/

MODULE_AUTHOR("Baptiste Pires, LIP6");
MODULE_LICENSE("GPL");


struct saakm_policy *central_policy;
static const char *policy_name = "saakm_central\0";

#define CENTRAL_CPU 0
#define DEFAULT_SLICE 2000000LU

/*
 * Struct representing a central entity.
 */
struct central_entity {
        int state; // entity state
        struct task_struct *task; // Pointer to the task_struct of this entity
        struct saakm_rq *rq; // IPANEMA runqueue if it's enqueued
        unsigned long slice_ns; // Time slice allocated
        unsigned long current_slice_ns; // Current time slice used
};

/*
 * Struct representing a central CPU.
 */
struct central_cpu {
        int cpu;
        enum saakm_core_state state; 
        struct central_entity *current_entity;
        struct saakm_rq rq;

        u64 started_at_ns;

        struct {
                unsigned long total_enqueue;
                unsigned long total_exec;
                unsigned long schedule_calls;
                unsigned long tick_calls;    
        } stats;
};
DEFINE_PER_CPU(struct central_cpu, cpus);

/*
 * Idle cpus to match default scx implementation
*/
struct central_idle_masks {
        cpumask_t cpus;
        cpumask_t smt;
} central_idle_cpus;

/*
        Start of cpus related functions
*/
static enum saakm_core_state central_get_core_state(struct saakm_policy *policy,
                                                struct core_event *e)
{
        int target = e->target;
        return per_cpu(cpus, target).state;
}

static void central_core_entry(struct saakm_policy *policy,
                                struct core_event *e)
{
        int cpu = e->target;
        per_cpu(cpus, cpu).state = SAAKM_ACTIVE_CORE;
}

static void central_core_exit(struct saakm_policy *policy,
                                struct core_event *e)
{
        int cpu = e->target;
        per_cpu(cpus, cpu).state = SAAKM_ACTIVE_CORE;
        cpumask_clear_cpu(cpu, &central_idle_cpus.cpus);
        cpumask_clear_cpu(cpu, &central_idle_cpus.smt);
}

static void central_newly_idle(struct saakm_policy *policy,
                                struct core_event *e)
{
        // TODO : check comment copier scx ici
}

static void central_entry_idle(struct saakm_policy *policy,
                                struct core_event *e)
{
        int cpu = e->target;
        per_cpu(cpus, cpu).state = SAAKM_IDLE_CORE;
        cpumask_set_cpu(cpu, &central_idle_cpus.cpus);


        /*
         * To match scx, if all logical SMTs are idle, set it in them mask
        */
        if (cpumask_subset(cpu_smt_mask(cpu), &central_idle_cpus.cpus)) {
                cpumask_or(&central_idle_cpus.smt,
                                &central_idle_cpus.smt,
                                cpu_smt_mask(cpu));
        }
}

static void central_exit_idle(struct saakm_policy *policy,
                                struct core_event *e)
{
        int cpu = e->target;
        per_cpu(cpus, cpu).state = SAAKM_ACTIVE_CORE;
        cpumask_clear_cpu(cpu, &central_idle_cpus.cpus);

        /*
         * To match scx, if one logical SMTs are not idle anymore,
         * clear it from the smt idle mask
        */
        cpumask_andnot(&central_idle_cpus.smt,
                        &central_idle_cpus.smt,
                        cpu_smt_mask(cpu));
}

/*
 * TODO : check if needed.
*/
static int central_balancing_select(struct saakm_policy *policy,
                                        struct core_event *e)
{
        return 0;
}

/*
        Start of entities related functions
*/

static int select_cpu(struct task_struct *p, int prev_cpu, int wake_flags)
{
        return CENTRAL_CPU;
}

static void central_enqueue_task(struct central_entity *ce,
                                struct saakm_rq *next_rq, int state)
{
        int cpu = CENTRAL_CPU;
        struct central_cpu *cc = &per_cpu(cpus, cpu);

        // if (next_rq->cpu != CENTRAL_CPU) {
                // pr_err("central_enqueue_task: trying to enqueue task on non-central cpu rq\n");
                // return;
        // }

        /*
         * When switching from SAAKM_RUNNING to SAAKM_READY_TICK the task
         * was removed from the cpu, we need to update the core.
         */
        if (state == SAAKM_READY_TICK) {
                cc->current_entity = NULL;
        }

        cc->stats.total_enqueue++;
        ce->state = state;
        ce->rq = next_rq;

        change_state(ce->task, state, cpu, next_rq);
}

static int central_new_prepare(struct saakm_policy *policy,
                                struct process_event *e)
{
        struct central_entity *ce;
        struct task_struct *p = e->target;

        ce = kzalloc(sizeof(struct central_entity), GFP_ATOMIC);

        if (!ce)
                return -1;

        ce->task = p;
        ce->rq = NULL;
        
        p->saakm.policy_metadata = ce;

        return select_cpu(p, task_cpu(p), e->flags);
}

static void central_new_place(struct saakm_policy *policy,
                                struct process_event *e)
{
        struct central_entity *ce = policy_metadata(e->target);
        int cpu_dst = task_cpu(ce->task);

        central_enqueue_task(ce, &per_cpu(cpus, cpu_dst).rq, SAAKM_READY);
}

static void central_new_end(struct saakm_policy *policy,
                            struct process_event *e)
{
}

static void central_block(struct saakm_policy *policy,
                            struct process_event *e)
{
        struct central_entity *ce = policy_metadata(e->target);
        int cpu = task_cpu(ce->task);
        struct central_cpu *cc = &per_cpu(cpus, cpu);

        ce->state = SAAKM_BLOCKED;
        ce->rq = NULL;
        cc->current_entity = NULL;

        change_state(ce->task, SAAKM_BLOCKED, cpu, NULL);
}

static int central_unblock_prepare(struct saakm_policy *policy,
                                struct process_event *e)
{
        return select_cpu(e->target, task_cpu(e->target), e->flags);
}

static void central_unblock_place(struct saakm_policy *policy,
                                struct process_event *e)
{
        struct central_entity *ce = policy_metadata(e->target);
        int cpu_dst = task_cpu(ce->task);

        central_enqueue_task(ce, &per_cpu(cpus, cpu_dst).rq, SAAKM_READY);
}

static void central_unblock_end(struct saakm_policy *policy,
                                struct process_event *e)
{
}

static void central_terminate(struct saakm_policy *policy,
                            struct process_event *e)
{
        struct central_entity *ce = policy_metadata(e->target);
        int cpu = task_cpu(ce->task);
        struct central_cpu *cc = &per_cpu(cpus, cpu);

        if (ce->state == SAAKM_RUNNING) {
                cc->current_entity = NULL;
        }

        ce->state= SAAKM_TERMINATED;
        ce->rq = NULL;

        change_state(ce->task, SAAKM_TERMINATED, cpu, NULL);
        kfree(ce);
}

static void central_yield(struct saakm_policy *policy,
                            struct process_event *e)
{
        struct central_entity *ce = policy_metadata(e->target);
        central_enqueue_task(ce, &per_cpu(cpus, task_cpu(ce->task)).rq, SAAKM_READY);
}

static void central_tick(struct saakm_policy *policy,
                        struct process_event *e)
{
        struct central_entity *ce = policy_metadata(e->target);

        u64 now = ktime_get_ns();

        ce->current_slice_ns += now - ce->current_slice_ns;

        if (ce->current_slice_ns >= ce->slice_ns) {
                ce->current_slice_ns = 0;
                /* Tasks has used its whole quantum, make it READY instead of RUNNING. */
                central_enqueue_task(ce, &per_cpu(cpus, task_cpu(ce->task)).rq,
                                    SAAKM_READY_TICK);
        }

        per_cpu(cpus, task_cpu(ce->task)).stats.tick_calls++;
}

static void central_schedule(struct saakm_policy *policy, unsigned int cpu)
{
        struct task_struct *next, *curr = get_saakm_current(cpu);
        struct central_entity *ce;
        struct central_cpu *cc = &per_cpu(cpus, cpu);

        if (curr) {
                struct central_entity *ccurr = policy_metadata(curr);
                per_cpu(cpus, cpu).stats.total_exec += ccurr->current_slice_ns;

                pr_warn("Scheduling while a task is running on cpu %d\n", cpu);
        }

        next = saakm_first_task(&cc->rq);

        if (!next && cpu != CENTRAL_CPU) {
                return;
        }

        ce = policy_metadata(next);
        ce->state = SAAKM_RUNNING;
        ce->rq = NULL;
        ce->current_slice_ns = 0;
        ce->slice_ns = DEFAULT_SLICE;

        cc->current_entity = ce;
        cc->stats.schedule_calls++;

        change_state(ce->task, SAAKM_RUNNING, cpu, NULL);

}

static int central_init(struct saakm_policy *policy)
{
        return 0;
}

static int central_free_metadata(struct saakm_policy *policy)
{
        if (policy->data)
                kfree(policy->data);
        return 0;
}

static int central_can_be_default(struct saakm_policy *policy)
{
        return 1;
}

static bool central_attach(struct saakm_policy *policy,
                                struct task_struct *task,
                                char *command)
{
        return true;
}

struct saakm_module_routines central_routines = {

        /* CPUs related functions */
        .get_core_state = central_get_core_state,
        .core_entry = central_core_entry,
        .core_exit = central_core_exit,
        .newly_idle = central_newly_idle,
        .enter_idle = central_entry_idle,
        .exit_idle = central_exit_idle,
        .balancing_select = central_balancing_select,

        /* Entities related functions */
        .new_prepare = central_new_prepare,
        .new_place = central_new_place,
        .new_end = central_new_end,
        .terminate = central_terminate,
        .block = central_block,
        .unblock_prepare = central_unblock_prepare,
        .unblock_place = central_unblock_place,
        .unblock_end = central_unblock_end,
        .yield = central_yield,
        .tick = central_tick,
        .schedule = central_schedule,

        /* Policy related functions */
        .init = central_init,
        .free_metadata = central_free_metadata,
        .can_be_default = central_can_be_default,
        .attach = central_attach
};


static struct hrtimer central_timer;
static ktime_t kt_period;

static enum hrtimer_restart central_timer_callback(struct hrtimer *timer)
{
        // u64 now_ns = ktime_get_ns();
        int local_cpu = smp_processor_id();
        // int cpu;

        if (local_cpu != CENTRAL_CPU) {
                // hrtimer_forward_now(&central_timer, kt_period);
                pr_err("central_timer_callback called on non-central cpu %d\n", local_cpu);
                return HRTIMER_NORESTART;
        }

        // for_each_possible_cpu(cpu) {
                // if (cpu == CENTRAL_CPU)
                        // continue;
                // 
                // 
                // 
        // }

        hrtimer_forward_now(&central_timer, kt_period);
        return HRTIMER_RESTART;
}

static int __init saakm_central_init(void)
{
        int res, cpu;
        struct central_cpu *c;
        
        pr_info("Loading saakm_central module\n");

        for_each_possible_cpu(cpu) {
                c = &per_cpu(cpus, cpu);
                c->cpu = cpu;
                c->state = SAAKM_ACTIVE_CORE;
                
                init_saakm_rq(&c->rq, FIFO, cpu, SAAKM_READY, NULL);

                c->stats.total_enqueue = 0;
		c->stats.total_exec = 0;
		c->stats.schedule_calls = 0;
		c->stats.tick_calls = 0;
        }

        central_policy = kzalloc(sizeof(struct saakm_policy), GFP_KERNEL);

        if (!central_policy) {
                pr_err("saakm_central: unable to allocate policy\n");
                return -ENOMEM;
        }

        strncpy(central_policy->name, policy_name, MAX_POLICY_NAME_LEN);
        central_policy->kmodule = THIS_MODULE;
        central_policy->routines = &central_routines;


        res = saakm_add_policy(central_policy);

        if (res) {
                kfree(central_policy);
        }

        
        kt_period = ms_to_ktime(1000); // 1 ms
        hrtimer_init(&central_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL_SOFT);
        central_timer.function = &central_timer_callback;
        hrtimer_start(&central_timer, kt_period, HRTIMER_MODE_REL_SOFT);
        
        return res;
}

module_init(saakm_central_init);


static void __exit saakm_central_exit(void)
{
        hrtimer_cancel(&central_timer);
        saakm_remove_policy(central_policy);        
        kfree(central_policy);
        pr_info("Unloading saakm_central module\n");
}
module_exit(saakm_central_exit);