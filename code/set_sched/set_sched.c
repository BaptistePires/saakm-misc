
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/syscall.h>

#define SCHED_SCX	7
#define SCHED_IPANEMA	8

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
};

static int sched_setattr(pid_t pid, const struct sched_attr *attr,
			 unsigned int flags)
{
	return syscall(SYS_sched_setattr, pid, attr, flags);
}

static inline void usage()
{
	fprintf(stderr,
		"Usage:\n"
		"\t set_sched scheduling_class program [args]\n"
		"\n"
		"\t scheduling_class	7 for SCX, 8 for SaaKM\n"
		"\t program	the program to launch\n"
		"\t args	arguments for program\n");
}

int main(int argc, char **argv)
{
	int ret;
	unsigned int sched_class = SCHED_IPANEMA;
	char *tmp;
	struct sched_attr attr = {
		.size = sizeof(struct sched_attr),
		.sched_policy = 0,
		.sched_flags = 0,
		.sched_nice = 0,
		.sched_priority = 0,
		.sched_ipa_policy = 0,
		.sched_ipa_attr_size = 0,
		.sched_ipa_attr = NULL,
	};

	if (argc < 3)
		goto bad_usage;

	sched_class = strtol(argv[1], &tmp, 10);
	if (argv[1] == tmp)
		goto bad_usage;
	attr.sched_ipa_policy = 0;

	ret = sched_setattr(0, &attr, 0);
	if (ret < 0) {
		perror("sched_setattr() failed");
		goto end;
	}

	ret = execvp(argv[2], argv+2);
	perror("execvp() failed");

end:
	return EXIT_FAILURE;

bad_usage:
	usage();
	return EXIT_FAILURE;
}
