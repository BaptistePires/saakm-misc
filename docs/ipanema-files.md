# branch : 5.4.1
### ./include/generated/autoconf.h
`+ #define CONFIG_CGROUP_IPANEMA 1`
Pour rajouter les options de compilation

###  ./include/linux/cgroup_subsys.h
```
/* And yet... */
#if IS_ENABLED(CONFIG_CGROUP_IPANEMA)
SUBSYS(ipanema)
#endif
```
Ajout d'un sous-système `cgroup` ipanema.

### ./include/linux/interrupt.h
```c
enum
{
	HI_SOFTIRQ=0,
	TIMER_SOFTIRQ,
	NET_TX_SOFTIRQ,
	NET_RX_SOFTIRQ,
	BLOCK_SOFTIRQ,
	IRQ_POLL_SOFTIRQ,
	TASKLET_SOFTIRQ,
	SCHED_SOFTIRQ,
+	SCHED_SOFTIRQ_IPANEMA,
	HRTIMER_SOFTIRQ, /* Unused, but kept as tools rely on the
			    numbering. Sigh! */
	RCU_SOFTIRQ,    /* Preferable RCU should always be the last softirq */

	NR_SOFTIRQS
};
```
Ajout de la soft IRQ pour ipanema.


### ./include/linux/ipanema.h
Header pour ipanema.

### ./include/linux/sched.h
- Definition des états d'IPANEMA
- Definition de struct `ipanema_entity`
```c
struct sched_ipanema_entity {
	union {
		struct rb_node node_runqueue;
		struct list_head node_list;
	};

	/* used for load balancing in policies */
	struct list_head ipa_tasks;

	int just_yielded;
	int nopreempt;

	enum ipanema_state state;
	struct ipanema_rq *rq;

	/* Policy-specific metadata */
	void *policy_metadata;

	struct ipanema_policy *policy;
};
```
- Ajout de `ipanema_entity` a `struct task_struct`.

### ./include/uapi/linux/sched.h
- Ajout la scheduling policy d'ipa. avec comme id 7


### ./include/uapi/linux/sched/types.h
Modification de la struct `sched_attr` pour ajouter les champs utilisés
par ipanema :
```c
	/* SCHED_IPANEMA */
	__u32 sched_ipa_policy;
	__u32 sched_ipa_attr_size;
	void  *sched_ipa_attr;
```

### ./kernel/sched/core.c
- Modification de `do_set_cpus_allowed`
- Modification de `select_task_rq`
- Modification de `scheduler_ipi` pour raise softirq
- Modification de `sched_fork` pour set la bonne sched\_class
_ Modification de `scheduler_tick` pour trigger lb 
...

### ./kernel/sched/fair.c
- Pour le NUMA balancing :
```c
fuckoff_cfs:
		ret = migrate_task_to(p, env.best_cpu);
		WRITE_ONCE(best_rq->numa_migrate_on, 0);
		if (ret != 0)
			trace_sched_stick_numa(p, env.src_cpu, env.best_cpu);
		return ret;
	}

	if (env.best_task->sched_class == &ipanema_sched_class) {
		put_task_struct(env.best_task);
		goto fuckoff_c
```
- Eviter le load balancing quand il n'y a pas de task dans CFS mais qu'il y en a dans ipanema, dans la function `pick_next_task_fair` :
```c
	/*
	 * We added the SCHED_IPANEMA scheduling class as a lower priority
	 * policy than SCHED_NORMAL. This means that we can be here while
	 * SCHED_IPANEMA has tasks to schedule. Check this and perform idle
	 * balancing only if SCHED_IPANEMA has no runnable task
	 */
	if (rq->nr_ipanema_running)
		return NULL;
```
- Modif de la struct sched_class de cfs pour ajouter ipanema :`.next			= &ipanema_sched_class,`


- 
### ./kernel/sched/ipanema.c
- Implémentation d'ipanema

### ./kernel/sched/ipanema.h
- 

### ./kernel/sched/sched.h
- Check pour tester si policy == ipanema
- Definition des functions pour les param de sched
- Ajout de `nr_ipanema_running` dans la `struct rq`
- Ajout de `next_balance_ipanema` dans rq

### ./tools/include/uapi/linux/sched.h
- #define SCHED_IPANEMA           7

### ./tools/ipanema/ipasetpolicy/ipasetpolicy.c
- Programme user pour set la policy d'un PID.

### ./tools/ipanema/ipastart/ipastart.c
- Start un processus avec une policy d'ipanema.

### Différents algos implémentés avec ipanema
./kernel/sched/ipanema/cfs.c
./kernel/sched/ipanema/cfs_wwc.c
./kernel/sched/ipanema/cfs_wwc_local_new_unblock.c
