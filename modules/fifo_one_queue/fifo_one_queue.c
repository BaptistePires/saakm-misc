#include "fifo_one_queue.h"
#include "linux/cpumask.h"
#include "linux/gfp_types.h"
#include "linux/ipanema.h"
#include "linux/irqflags.h"
#include "linux/pci.h"
#include "linux/slab.h"
/**
    TODO : 
        - use a cpu_mask for the actual cpu presents.
*/


MODULE_AUTHOR("Baptiste Pires, LIP6");
MODULE_LICENSE("GPL");

// #define FIFO_DEBUG 1
/* We use a quantum of 30ms */
#define QUANTUM_NS 20 * NSEC_PER_MSEC
#define CORE_HAVE_CURRENT(c) ((c)->_current ? 1 : 0)

#define ipanema_rq_of(ipanema_rq) container_of((ipanema_rq), struct fifo_rq, rq)


struct ipanema_policy *fifo_policy;
static const char *policy_name = "fifo_one_queue";

/*
    Structure representing a fifo runqueue.
*/
struct fifo_rq {
    /* The actual Ipanema runqueue. */
    struct ipanema_rq rq;

    /* As we use only one runqueue, whenever we en/dequeue, 
        we need to lock it. */
    raw_spinlock_t lock;

};


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

static struct fifo_rq fifo_runqueue;

#define LOCK_RQ() raw_spin_lock(&fifo_runqueue.rq.lock);
#define UNLOCK_RQ() raw_spin_unlock(&fifo_runqueue.rq.lock);

DEFINE_PER_CPU(struct fifo_core, core);
DEFINE_PER_CPU(struct ipanema_rq, local_rq);

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
    pr_info("oui oui\n");
}

static void fifo_core_exit(struct ipanema_policy *policy, struct core_event *e)
{
    struct fifo_core *c = &per_cpu(core, e->target);
    c->state = IPANEMA_IDLE_CORE;
    cpumask_clear_cpu(c->id, &online_cores);
    cpumask_clear_cpu(c->id, idle_cores);
}

// static void fifo_newly_idle(struct ipanema_policy *policy, struct core_event *e)
// {

//     struct task_struct *next, *next_saved;
//     struct fifo_core *local_fifo_core = &per_cpu(core, e->target);
//     int cpu = e->target, prev_cpu;
//     unsigned long flags;

//     /* First, check local rq */
//     local_irq_save(flags);

// retry:
//     raw_spin_lock(&per_cpu(core,cpu).rq.lock);
//     LOCK_RQ();
//     next = ipanema_first_task(&fifo_runqueue.rq);
    
//     /* No ready task, we can leave */
//     if (!next) {
//         pr_info("no task to migarte\n");
//         goto unlock_local;
//     }
    
//     prev_cpu = task_cpu(next);

//     /* @next was scheduled on this core last time, we don't need to migrate it here,
//      * we can just queue it in our local rq.
//      */
//     if (prev_cpu == cpu) {
//         // pr_info("task found and was already on this cpu\n");
//         change_state(next, IPANEMA_READY, cpu, &local_fifo_core->rq);
//         goto unlock_local;
//     }

//     /* @next prev_cpu was not us, we need to migrate it to us */
//     next_saved = next;
//     UNLOCK_RQ();
//     raw_spin_unlock(&per_cpu(core,cpu).rq.lock);

//     LOCK_RQ();
//     ipanema_lock_core(prev_cpu);
    
//     next = ipanema_first_task(&fifo_runqueue.rq);

//     /* Next was scheduled on another CPU, we retry */
//     if (!next || next != next_saved) {
//         pr_info("task was scheduled on another cpu\n");
//         UNLOCK_RQ();
//         ipanema_unlock_core(prev_cpu);
//         goto retry;
//     }

//     /* We can start to migrate it */
//     change_state(next, IPANEMA_MIGRATING, cpu, NULL);
//     UNLOCK_RQ();
//     ipanema_unlock_core(prev_cpu);

//     /* Now we can queue it in our local rq to schedule it when we return */
//     raw_spin_lock(&per_cpu(core,cpu).rq.lock);
//     LOCK_RQ();
//     change_state(next, IPANEMA_READY, cpu, &local_fifo_core->rq);

//     pr_info("task migrated\n");
// unlock_local:
//     UNLOCK_RQ();
//     raw_spin_unlock(&per_cpu(core,cpu).rq.lock);
//     local_irq_restore(flags);
// }


static void fifo_newly_idle(struct ipanema_policy *policy, struct core_event *e)
{

    struct task_struct *next;
    struct fifo_core *local_fifo_core = &per_cpu(core, e->target), *remote_fifo_core;
    int cpu = e->target, prev_cpu;
    // unsigned long flags;

    /* First, check local rq */
    // local_irq_save(flags);
    
    LOCK_RQ();
    next = ipanema_first_task(&fifo_runqueue.rq);

    if (!next)
        goto unlock_global;

    if (task_cpu(next) == cpu) {
        raw_spin_lock(&local_fifo_core->rq.lock);
        change_state(next, IPANEMA_READY, cpu, &local_fifo_core->rq);
        raw_spin_unlock(&local_fifo_core->rq.lock);
        goto unlock_global;
    }

    prev_cpu = task_cpu(next);
    remote_fifo_core = &per_cpu(core, prev_cpu);

    raw_spin_lock(&remote_fifo_core->rq.lock);
    change_state(next, IPANEMA_MIGRATING, cpu, NULL);
    raw_spin_unlock(&remote_fifo_core->rq.lock);

    raw_spin_lock(&local_fifo_core->rq.lock);
    change_state(next, IPANEMA_READY, cpu, &local_fifo_core->rq);
    raw_spin_unlock(&local_fifo_core->rq.lock);

unlock_global:
    UNLOCK_RQ();
    // local_irq_restore(flags);
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

static void fifo_balancing_select(struct ipanema_policy *policy, struct core_event *e)
{
}

/* ------------------------ CPU cores related functions END ------------------------ */
/* ------------------------ Processes related // extern void ipanema_call_trace_select_cpu(int prev_cpu, int cpu, const struct cpumask *idle_cores);functions ------------------------ */
static int select_cpu(int prev_cpu)
{

    /* Find the first online and idle core, if there none matches, return previous cpu*/
    unsigned int cpu;

    LOCK_RQ();
    if (cpumask_test_cpu(prev_cpu, idle_cores))
        cpu = prev_cpu;
    else
        cpumask_any_but(idle_cores, prev_cpu);

    ipanema_call_trace_select_cpu(prev_cpu, cpu, idle_cores);
    
    if (cpu >= nr_cpu_ids)
        cpu = prev_cpu;
    
    if (cpumask_test_cpu(cpu, idle_cores)) {
        // pr_info("cpu %d cleared\n", cpu);
        cpumask_clear_cpu(cpu, idle_cores); 
    }
    
    UNLOCK_RQ();
    return cpu;
}

static void fifo_enqueue_lock(struct fifo_process *fp, enum ipanema_state state)
{
    struct task_struct *p = fp->task;
    struct fifo_core *fc = &per_cpu(core, task_cpu(p));

    LOCK_RQ();
    if (state == IPANEMA_RUNNING)
        fc->_current = NULL;
    change_state(p, state, task_cpu(p), &fifo_runqueue.rq);

    UNLOCK_RQ();
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
    fp->rq = NULL;
    p->ipanema.policy_metadata = fp;

    if (e->flags & IPANEMA_WF_EXEC)
        return task_cpu(p);

    return select_cpu(task_cpu(p));
}

/*
    This function is only called when a task is in a 
    IPANEMA_NOT_QUEUED state, right after "ipanema_new_prepare".
    
    We chosed a core to execute the task on before in "ipanema_new_prepare".
*/
static void fifo_new_place(struct ipanema_policy *policy, struct process_event *e)
{
    struct fifo_process *fp = policy_metadata(e->target);
    fifo_enqueue_lock(fp, IPANEMA_READY);
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
    
    fp->current_quantum_ns += 1;
    if ((fp->current_quantum_ns % QUANTUM_NS) == 0) {
        fifo_enqueue_lock(fp, IPANEMA_READY_TICK);
    }

}

static void fifo_yield(struct ipanema_policy *policy, struct process_event *e)
{
    struct fifo_process *fp = policy_metadata(e->target);
    fifo_enqueue_lock(fp, IPANEMA_READY);
}

static void fifo_block(struct ipanema_policy *policy, struct process_event *e)
{
    struct fifo_process *fp = policy_metadata(e->target);
    struct fifo_core *fc = &per_cpu(core, smp_processor_id());


    if (fp->state != IPANEMA_RUNNING) {
        pr_info("fp->state != IPANEMA_RUNNING! state=%d\n", fp->state);
    } else {
        fc->_current = NULL;
    }

    fp->state = IPANEMA_BLOCKED;
    fp->rq = NULL;

    LOCK_RQ();
    change_state(fp->task, fp->state, task_cpu(fp->task), NULL);
    UNLOCK_RQ()
}

/*
    The next two functions (fifo_unblock_prepare and fifo_unblock_place) follow the 
    same design as "new_prepare" and "new_place". The "prepare" step is to find a 
    cpu on which we can queue the task and place if for the actual queuing.
*/
static int fifo_unblock_prepare(struct ipanema_policy *policy, struct process_event *e)
{
    struct task_struct *p = e->target;
    return select_cpu(task_cpu(p));
}

static void fifo_unblock_place(struct ipanema_policy *policy, struct process_event *e)
{
    struct fifo_process *fp = policy_metadata(e->target);
    fifo_enqueue_lock(fp, IPANEMA_READY);
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

    fp->state = IPANEMA_TERMINATED;
    
    // raw_spin_lock(&fifo_runqueue.rq.lock);
    LOCK_RQ();
    change_state(fp->task, fp->state, task_cpu(fp->task), NULL);
    // raw_spin_unlock(&fifo_runqueue.rq.lock);
    UNLOCK_RQ();
    fp->rq = NULL;
    kfree(fp);
}


static void enqueue_running_task_fifo(struct task_struct *next, int cpu)
{
    struct fifo_process *fp = policy_metadata(next);
    struct fifo_core *fc = &per_cpu(core, cpu);

    fp->state = IPANEMA_RUNNING;
    fc->_current = fp;
    
    change_state(fp->task, fp->state, cpu, NULL);
}

static void fifo_schedule(struct ipanema_policy *policy, unsigned int cpu)
{
    struct task_struct *next;
    struct fifo_core *fc = &per_cpu(core, cpu);

    raw_spin_lock(&fc->rq.lock);
    /* first, check if we got a task in the local rq, only newly_idle can put one */
    next = ipanema_first_task(&fc->rq);

    /* no task in local rq, check if the global */
    if (!next) {
        raw_spin_unlock(&fc->rq.lock);
        goto check_global_rq;
    }

    // pr_info("task was in local rq\n");
    // enqueue_running_task_fifo(next, cpu);
    change_state(next, IPANEMA_RUNNING, cpu, NULL);
    raw_spin_unlock(&fc->rq.lock);
    return;

check_global_rq:
    LOCK_RQ();
    next = ipanema_first_task(&fifo_runqueue.rq);

    if (!next)
        goto unlock;

    /*
     * If the next task last cpu was not us, we will try to pick it in 
     * fifo_new_idle() after this fifo_schedule() call does not set 'current' for
     * this cpu.
     */
    if (task_cpu(next) == cpu) {
        enqueue_running_task_fifo(next, cpu);
    }

    // pr_info("first task was not scheduled here bf\n");
unlock: 
    UNLOCK_RQ();
}

static void fifo_load_balance(struct ipanema_policy *policy, struct core_event *e)
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
    Debug FS functions.
*/
static int fifo_full_debug_info(struct seq_file *m,  void *p)
{
    unsigned int nr;

    struct fifo_process *fp;
    struct task_struct *tsk;
    char *buffer = kvmalloc(1024, GFP_KERNEL);
    int nr_written;

    raw_spin_lock_irq(&fifo_runqueue.rq.lock);
    nr = fifo_runqueue.rq.nr_tasks;
    if (nr) {
        tsk =  list_first_entry(&fifo_runqueue.rq.head, struct task_struct, ipanema.node_list);
        fp = (struct fifo_process *) tsk->ipanema.policy_metadata;
        nr_written = snprintf(buffer, 1024, "\t\t #%u : pid=%d comm=%s state=%x ipanema_state=%x quantum=%lu\n", 0, tsk->pid, tsk->comm, tsk->__state, fp->state, fp->current_quantum_ns);
        // seq_printf(m, "\t\t #%u : pid=%d comm=%s state=%x ipanema_state=%x quantum=%lu\n", 0, tsk->pid, tsk->comm, tsk->__state, fp->state, fp->current_quantum_ns);
    }

    if (nr > 1) {
        tsk =  list_last_entry(&fifo_runqueue.rq.head, struct task_struct, ipanema.node_list);
        fp = (struct fifo_process *) tsk->ipanema.policy_metadata;
        
        nr_written += snprintf(buffer + nr_written, (1024 - nr_written), "...\n\t\t #%u : pid=%d comm=%s state=%x ipanema_state=%x quantum=%lu\n", nr - 1, tsk->pid, tsk->comm, tsk->__state, fp->state, fp->current_quantum_ns);
    }
    raw_spin_unlock_irq(&fifo_runqueue.rq.lock);



    seq_printf(m, "FIFO runqueue\n");
    seq_printf(m, "---------------------\n");
    seq_printf(m, "nr_tasks : %u\n", nr);
    seq_printf(m, "---------------------\n");

    if (nr_written) {
        seq_printf(m, "%s", buffer);
    }
    // for_each_possible_cpu(cpu) {
    //     ipanema_lock_core(cpu);
    //     fc = &per_cpu(core, cpu);
    //     rq = &fc->rq;
    //     i = 0; 

    //     seq_printf(m, "cpu :%d\n", cpu);
    //     seq_printf(m, "---------------------\n");
    //     seq_printf(m, "fifo_core struct\n");
    //     seq_printf(m, "---------------------\n");
    //     seq_printf(m, "\t _current : %p\n", fc->_current);
    //     seq_printf(m, "\t state    : %d\n", fc->state);
    //     seq_printf(m, "---------------------\n");
    //     seq_printf(m, "ipanema_rq : \n");
    //     seq_printf(m, "---------------------\n");
    //     seq_printf(m, "\t nr_tasks : %x\n", rq->nr_tasks);
    //     seq_printf(m, "\t state : %x\n", rq->state);

    //     list_for_each_entry(tsk, &rq->head, ipanema.node_list) {
    //         fp = policy_metadata(tsk);
    //         seq_printf(m, "\t\t #%u : pid=%d comm=%s state=%x ipanema_state=%x quantum=%lu\n", i, tsk->pid, tsk->comm, tsk->__state, fp->state, fp->current_quantum_ns);
    //     }
    //     raw_spin_unlock(&per_cpu(core,cpu).rq);
    // }

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
    init_ipanema_rq(&fifo_runqueue.rq, FIFO, 0, IPANEMA_READY, NULL);
    raw_spin_lock_init(&fifo_runqueue.lock);
    
    idle_cores = kzalloc(cpumask_size(), GFP_KERNEL);

    if (!idle_cores) {
        pr_info("Failed to allocate memory for idle_cores\n");
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
        init_ipanema_rq(&c->rq, FIFO, cpu, IPANEMA_READY, NULL);
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
