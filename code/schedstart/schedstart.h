#ifndef SCHEDSTART_H
#define SCHEDSTART_H

#include <stdint.h>
#include <unistd.h>

#define SCHED_EXT       7
#define SCHED_IPANEMA   8

struct sched_attr {
	uint32_t size;

	uint32_t sched_policy;
	uint64_t sched_flags;

	/* SCHED_NORMAL, SCHED_BATCH */
	int32_t sched_nice;

	/* SCHED_FIFO, SCHED_RR */
	uint32_t sched_priority;

	/* SCHED_DEADLINE */
	uint64_t sched_runtime;
	uint64_t sched_deadline;
	uint64_t sched_period;

	/* SCHED_IPANEMA */
	uint32_t sched_ipa_policy;
	uint32_t sched_ipa_attr_size;
	void *sched_ipa_attr;

        /* Utilization hints */
	uint32_t sched_util_min;
	uint32_t sched_util_max;      
};

static void usage(char **);
static int sys_sched_setattr(pid_t target, struct sched_attr *attr, unsigned int flags);
static unsigned int get_scheduling_policy_from_string(const char *sched);
#endif