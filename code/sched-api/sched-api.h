#ifndef SCHED_API_H
#define SCHED_API_H

#include <pthread.h>


#define SCHED_NORMAL 0
#define SCHED_IPANEMA 8

#define NR_THREADS 10

struct user_sched_attr {
	unsigned int size;

	unsigned int sched_policy;
	unsigned long long sched_flags;

	/* SCHED_NORMAL, SCHED_BATCH */
	int sched_nice;

	/* SCHED_FIFO, SCHED_RR */
	unsigned int sched_priority;

	/* SCHED_DEADLINE */
	unsigned long long sched_runtime;
	unsigned long long sched_deadline;
	unsigned long long sched_period;

	/* SCHED_IPANEMA */
	unsigned int sched_ipa_policy;
	unsigned int sched_ipa_attr_size;
	void  *sched_ipa_attr;

	/* Utilization hints */
	unsigned int sched_util_min;
	unsigned int sched_util_max;
};


int get_scheduling_policy(void);
void set_ipanema_scheduler(void); 
void set_eevdf_scheduler(void);
void set_ipanema_scheduler_with_pthread(void);

void child_process_fn(void);

void *thread_function(void *);
void threads_test(void);

void thread_setting_scheduler(unsigned int);
void *thread_setting_sched_fn(void*);

struct thread_info {
        pthread_t tid;
        unsigned int index;
        unsigned int data;
};

#endif