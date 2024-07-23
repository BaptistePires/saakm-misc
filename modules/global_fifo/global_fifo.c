#include "global_fifo.h"
#include "linux/cpumask.h"
#include "linux/gfp_types.h"
#include "linux/ipanema.h"
#include "linux/irqflags.h"
#include "linux/list.h"
#include "linux/pci.h"
#include "linux/sched.h"
#include "linux/slab.h"
/**
    TODO : 
        - use a cpu_mask for the actual cpu presents.
*/


MODULE_AUTHOR("Baptiste Pires, LIP6");
MODULE_LICENSE("GPL");

// #define FIFO_DEBUG 1
/* We use a quantum of 30ms */
#define QUANTUM_NS (20 * NSEC_PER_MSEC)
#define CORE_HAVE_CURRENT(c) ((c)->_current ? 1 : 0)

#define ipanema_rq_of(ipanema_rq) container_of((ipanema_rq), struct fifo_rq, rq)


struct ipanema_policy *fifo_policy;
static const char *policy_name = "fifo_one_queue";

/*
    Structure representing a fifo runqueue.
*/
struct global_fifo_runqueue {
    /* The actual Ipanema runqueue. */
    struct ipanema_rq rq;

    /* As we use only one runqueue, whenever we en/dequeue, 
        we need to lock it. */
    raw_spinlock_t lock;
} global_fifo_runqueue;;

#define LOCK_GLOBAL_RQ() raw_spin_lock(&global_fifo_runqueue.lock);
#define UNLOCK_GLOBAL_RQ() raw_spin_unlock(&global_fifo_runqueue.lock);

/*
    Represents a process.
*/
struct fifo_process {
    int state;
    struct task_struct *task;
    struct fifo_rq *rq;
    unsigned long current_quantum_ns;
    unsigned long last_sched_ns;
    
};

/*
    Represents a core.
*/
struct fifo_core {
    enum ipanema_core_state state; /* core state */
    int id; /* core id */
    struct ipanema_rq rq;
    struct fifo_process *_current; /* the current process executing on this core */

};

static cpumask_t online_cores;
static struct cpumask *idle_cores;

struct {
    cpumask_t cpu;
    cpumask_t smt;
} idle_masks;

DEFINE_PER_CPU(struct fifo_core, core);


/* ------------------------ CPU cores related functions ------------------------ */

static enum ipanema_core_state fifo_get_core_state(struct ipanema_policy *policy,
                struct core_event *e)
{
    int target = e->target;
    return per_cpu(core, target).state;
}

static void fifo_core_entry(struct ipanema_policy *policy, struct core_event *e)
{
    struct fifo_core *c = &per_cpu(core, e->target);
    c->state = IPANEMA_ACTIVE_CORE;
    cpumask_set_cpu(c->id, &online_cores);
    cpumask_set_cpu(c->id, idle_cores);
    // pr_info("oui oui\n");
}

static void fifo_core_exit(struct ipanema_policy *policy, struct core_event *e)
{
    struct fifo_core *c = &per_cpu(core, e->target);
    c->state = IPANEMA_IDLE_CORE;
    cpumask_clear_cpu(c->id, &online_cores);
    cpumask_clear_cpu(c->id, idle_cores);
}


static void fifo_newly_idle(struct ipanema_policy *policy, struct core_event *e)
{
    struct task_struct *next;
    struct fifo_process *fp;
    unsigned int local_cpu = e->target, current_task_cpu;
    
    
    LOCK_GLOBAL_RQ();
    next = ipanema_first_task(&global_fifo_runqueue.rq);

    if (!next){
        UNLOCK_GLOBAL_RQ();
        return;
    }

    fp = policy_metadata(next);
    ipanema_remove_task(&global_fifo_runqueue.rq, next);
    UNLOCK_GLOBAL_RQ();


    current_task_cpu = task_cpu(next);

    if (local_cpu == current_task_cpu)
        goto local;

    ipanema_double_lock_core(local_cpu, current_task_cpu);

    /* Fist we migrate it */
    change_state(next, IPANEMA_MIGRATING, local_cpu, NULL);

    /* we can release task's core */

    change_state(next, IPANEMA_READY, local_cpu, &per_cpu(core, local_cpu).rq);
    ipanema_unlock_core(current_task_cpu);

    ipanema_unlock_core(local_cpu);

    // pr_info("Moved one task to local rq :)\n");
    goto unlock;
    /* In the local case, we just add it to our local queue and it will be picked in schedule(). */
local:
    change_state(next, IPANEMA_READY, task_cpu(next), &per_cpu(core, local_cpu).rq);
unlock:
    
    
}
static void fifo_enter_idle(struct ipanema_policy *policy, struct core_event *e)
{
    per_cpu(core, e->target).state = IPANEMA_IDLE_CORE;
    cpumask_set_cpu(e->target, idle_cores);
}

static void fifo_exit_idle(struct ipanema_policy *policy, struct core_event *e)
{
    per_cpu(core, e->target).state = IPANEMA_ACTIVE_CORE;
    cpumask_clear_cpu(e->target, &online_cores);
    cpumask_clear_cpu(e->target, idle_cores);
}

/* ------------------------ CPU cores related functions END ------------------------ */
/* ------------------------ Processes related // extern void ipanema_call_trace_select_cpu(int prev_cpu, int cpu, const struct cpumask *idle_cores);functions ------------------------ */


static int select_cpu(struct task_struct *p, int prev_cpu)
{
    return p->pid % num_online_cpus();
}

/*
    This function is called in "ipanema_new_prepare".
    We can be called in the following scenario :
        1. We're switching class
        2. In "select_task_rq_ipanema" when we're forked.

    The goal of this function is to initialize the 
    entity that will be used by this scheduler to schedule 
    this task and to find the cpu on which it should starts 
    its execution.
*/
static int fifo_new_prepare(struct ipanema_policy *policy,
                struct process_event *e)
{
    struct fifo_process *fp;
    struct task_struct *p = e->target;

    fp = kzalloc(sizeof(struct fifo_process), GFP_ATOMIC);
    if (!fp)
        return -1;

    fp->task = p;
    fp->state = IPANEMA_NOT_QUEUED;
    
    p->ipanema.policy_metadata = fp;
    return select_cpu(p, task_cpu(p));
}

/*
    This function is only called when a task is in a 
    IPANEMA_NOT_QUEUED state, right after "ipanema_new_prepare".
    
    We chosed a core to execute the task on before in "ipanema_new_prepare".
*/
static void fifo_new_place(struct ipanema_policy *policy, struct process_event *e)
{
    LOCK_GLOBAL_RQ();
    change_state(e->target, IPANEMA_READY, task_cpu(e->target), &global_fifo_runqueue.rq);
    UNLOCK_GLOBAL_RQ();
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

    fp->current_quantum_ns += fp->last_sched_ns - now;
    fp->last_sched_ns = now;

    if ((fp->current_quantum_ns) >= QUANTUM_NS) {
        fp->current_quantum_ns = 0;

        // copy scx, if local queue empty, queue it back 
        if (list_empty(&per_cpu(core, e->cpu).rq.head)) {
            change_state(e->target, IPANEMA_READY_TICK, task_cpu(e->target), &per_cpu(core, e->cpu).rq);
            return;    
        }
        
        LOCK_GLOBAL_RQ();
        change_state(e->target, IPANEMA_READY_TICK, task_cpu(e->target), &global_fifo_runqueue.rq);
        UNLOCK_GLOBAL_RQ();
    }
}

static void fifo_yield(struct ipanema_policy *policy, struct process_event *e)
{
    LOCK_GLOBAL_RQ();
    change_state(e->target, IPANEMA_READY, task_cpu(e->target), &global_fifo_runqueue.rq);
    UNLOCK_GLOBAL_RQ();
}

static void fifo_block(struct ipanema_policy *policy, struct process_event *e)
{
    LOCK_GLOBAL_RQ();
    change_state(e->target, IPANEMA_BLOCKED, task_cpu(e->target), NULL);
    UNLOCK_GLOBAL_RQ();
}

/*
    The next two functions (fifo_unblock_prepare and fifo_unblock_place) follow the 
    same design as "new_prepare" and "new_place". The "prepare" step is to find a 
    cpu on which we can queue the task and place if for the actual queuing.
*/
static int fifo_unblock_prepare(struct ipanema_policy *policy, struct process_event *e)
{
    return task_cpu(e->target);
}

static void fifo_unblock_place(struct ipanema_policy *policy, struct process_event *e)
{
        // copy scx, if local queue empty, queue it back 
    // if (list_empty(&per_cpu(core, e->cpu).rq.head)) {
    //     change_state(e->target, IPANEMA_READY, task_cpu(e->target), &per_cpu(core, e->cpu).rq);
    //     return;    
    // }
    LOCK_GLOBAL_RQ();
    change_state(e->target, IPANEMA_READY, global_fifo_runqueue.rq.cpu, &global_fifo_runqueue.rq);
    UNLOCK_GLOBAL_RQ();
}

static void fifo_unblock_end(struct ipanema_policy *policy, struct process_event *e)
{

}

/*
    This functionis only called in "ipanema_terminate".
    We can be called in those scenario :
        1. We're switching out from ipanema
        2. The task is dying, therefore we need to remove it.
*/
static void fifo_terminate(struct ipanema_policy *policy, struct process_event *e)
{
    struct fifo_process *fp = policy_metadata(e->target);
    struct fifo_core *fc = &per_cpu(core, task_cpu(e->target));

    if (fp->state == IPANEMA_RUNNING) {
        fc->_current = NULL;
    }
    fp->state = IPANEMA_TERMINATED;
    fp->rq = NULL;

    change_state(e->target, IPANEMA_TERMINATED, task_cpu(e->target), NULL);
}

static void fifo_schedule(struct ipanema_policy *policy, unsigned int cpu)
{
    struct task_struct *next;
    struct fifo_process *fp;
    struct fifo_core *fc = &per_cpu(core, cpu);

    /* first we check local rq */
    next = ipanema_first_task(&per_cpu(core, cpu).rq);

    if (next) {
        // pr_info("found one :)\n");
        fp = policy_metadata(next);
        fp->state = TASK_RUNNING;
        fp->rq = NULL;

        fp->last_sched_ns = ktime_get_ns();
        fp->current_quantum_ns = 0;

        fc->_current = fp;
        change_state(next, IPANEMA_RUNNING, cpu, NULL);
        return;
    }

    LOCK_GLOBAL_RQ();
    next = ipanema_first_task(&global_fifo_runqueue.rq);

    if (!next || (task_cpu(next) != cpu)) {
        goto unlock;
    }

    fp = policy_metadata(next);
    fp->state = TASK_RUNNING;
    fp->rq = NULL;
    fp->last_sched_ns = ktime_get_ns();
    fp->current_quantum_ns = 0;

    fc->_current = fp;

    change_state(next, IPANEMA_RUNNING, cpu, NULL);

unlock:
    UNLOCK_GLOBAL_RQ();
}

static void fifo_load_balance(struct ipanema_policy *policy, struct core_event *e)
{

    fifo_newly_idle(policy, e);
//     struct task_struct *next;
//     unsigned int local_cpu = e->target, current_task_cpu;

//     LOCK_GLOBAL_RQ();
//     next = ipanema_first_task(&global_fifo_runqueue.rq);

//     if (!next)
//         goto unlock;

//     current_task_cpu = task_cpu(next);

//     if (local_cpu == current_task_cpu)
//         goto local;

//     ipanema_double_lock_core(local_cpu, current_task_cpu);

//     /* Fist we migrate it */
//     change_state(next, IPANEMA_MIGRATING, local_cpu, NULL);

//     /* we can release task's core */
//     ipanema_unlock_core(current_task_cpu);

//     change_state(next, IPANEMA_READY, local_cpu, &per_cpu(core, local_cpu).rq);
//     ipanema_unlock_core(local_cpu);

//     // pr_info("Moved one task to local rq :)\n");
//     goto unlock;
//     /* In the local case, we just add it to our local queue and it will be picked in schedule(). */
// local:
//     change_state(next, IPANEMA_READY, task_cpu(next), &per_cpu(core, local_cpu).rq);
// unlock:
//     UNLOCK_GLOBAL_RQ();
    
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
    Debug FS functions.
*/
static int fifo_full_debug_info(struct seq_file *m,  void *p)
{
    unsigned int nr;

    struct fifo_process *fp;
    struct task_struct *tsk;
    char *buffer = kvmalloc(1024, GFP_KERNEL);
    int nr_written;

    raw_spin_lock_irq(&global_fifo_runqueue.lock);
    nr = global_fifo_runqueue.rq.nr_tasks;
    if (nr) {
        tsk =  list_first_entry(&global_fifo_runqueue.rq.head, struct task_struct, ipanema.node_list);
        fp = (struct fifo_process *) tsk->ipanema.policy_metadata;
        nr_written = snprintf(buffer, 1024, "\t\t #%u : pid=%d comm=%s state=%x ipanema_state=%x quantum=%lu\n", 0, tsk->pid, tsk->comm, tsk->__state, fp->state, fp->current_quantum_ns);
        // seq_printf(m, "\t\t #%u : pid=%d comm=%s state=%x ipanema_state=%x quantum=%lu\n", 0, tsk->pid, tsk->comm, tsk->__state, fp->state, fp->current_quantum_ns);
    }

    if (nr > 1) {
        tsk =  list_last_entry(&global_fifo_runqueue.rq.head, struct task_struct, ipanema.node_list);
        fp = (struct fifo_process *) tsk->ipanema.policy_metadata;
        
        nr_written += snprintf(buffer + nr_written, (1024 - nr_written), "...\n\t\t #%u : pid=%d comm=%s state=%x ipanema_state=%x quantum=%lu\n", nr - 1, tsk->pid, tsk->comm, tsk->__state, fp->state, fp->current_quantum_ns);
    }
    raw_spin_unlock_irq(&global_fifo_runqueue.lock);



    seq_printf(m, "FIFO runqueue\n");
    seq_printf(m, "---------------------\n");
    seq_printf(m, "nr_tasks : %u\n", nr);
    seq_printf(m, "---------------------\n");

    if (nr_written) {
        seq_printf(m, "%s", buffer);
    }

    return 0;
}

struct proc_dir_entry *proc_entry;
static int __init fifo_mod_init(void)
{
    int res;
    int cpt = 0;
    int cpu;
    struct fifo_core *c;


    /* Init global rq */
    init_ipanema_rq(&global_fifo_runqueue.rq, FIFO, 0, IPANEMA_READY, 1, NULL);
    raw_spin_lock_init(&global_fifo_runqueue.lock);
    
    idle_cores = kzalloc(cpumask_size(), GFP_KERNEL);

    if (!idle_cores) {
        // pr_info("Failed to allocate memory for idle_cores\n");
        return -ENOMEM;
    }
       

    cpumask_clear(idle_cores);
    cpumask_clear(&online_cores);
    /* Init cores */
    for_each_possible_cpu(cpu) {
        c = &per_cpu(core, cpu);
        c->id = cpu;
        c->state = IPANEMA_ACTIVE_CORE;
        cpumask_set_cpu(cpu, &online_cores);
        cpumask_set_cpu(cpu, idle_cores);
        init_ipanema_rq(&c->rq, FIFO, cpu, IPANEMA_READY, 0, NULL);
        /* No need for an order function as we're  FIFO. */
        ++cpt;
    }

    pr_info("Created %d runqueues, idle_cpu masks : 0x%*pb\n", cpt, cpumask_pr_args(idle_cores));


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

    proc_entry = proc_create_single("fifo_debug", 0, NULL, fifo_full_debug_info);
    return res;
}
module_init(fifo_mod_init);

static void __exit fifo_exit(void)
{
    int res;

    res = ipanema_remove_policy(fifo_policy);

    kfree(fifo_policy);
    
    proc_remove(proc_entry);
    printk(KERN_INFO "FIFO module unloaded\n");
    
}
module_exit(fifo_exit);
