#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <string.h>


#include "schedstart.h"


int main(int argc, char **argv)
{

        char *strscheduler = argv[1], *end;
        int ipanema_scheduling_policy;
        unsigned int scheduling_policy;
        int ret;
        

        if (argc < 3) {
                usage(argv);
                return EXIT_FAILURE;
        }

        scheduling_policy = get_scheduling_policy_from_string(strscheduler);
        if (!scheduling_policy) {
                fprintf(stderr, "Unknown scheduler: %s\n", strscheduler);
                usage(argv);
                return EXIT_FAILURE;
        }


        // TODO better checks
        errno = 0;
        ipanema_scheduling_policy = strtol(argv[2], &end, 10);
        if (argv[2] == end) {
                fprintf(stderr, "Invalid scheduling policy: %s\n", argv[2]);
                usage(argv);
                return EXIT_FAILURE;
        }

        struct sched_attr attr = {
                .size                   = sizeof(attr),
                .sched_policy           = scheduling_policy,
                .sched_flags            = 0,
                .sched_nice             = 0,
                .sched_priority         = 0,
                .sched_runtime          = 0,
                .sched_deadline         = 0,
                .sched_period           = 0,

                .sched_ipa_policy       = ipanema_scheduling_policy,
                .sched_ipa_attr_size    = 0,
                .sched_ipa_attr         = NULL,

                .sched_util_min         = 0,
                .sched_util_max         = 0
        };

        ret = sys_sched_setattr(0, &attr, 0);

        if (ret < 0) {
                fprintf(stderr, "Failed to set scheduling policy %s: %s\n",
                        strscheduler, strerror(errno));
                        perror("sched_setattr");
                return EXIT_FAILURE;
        }

        ret = execvp(argv[3], &argv[3]);
        if (ret < 0) {
                fprintf(stderr, "Failed to execute program %s: %s\n",
                        argv[3], strerror(errno));
                perror("execvp");
                return EXIT_FAILURE;
        }


        return EXIT_SUCCESS;
}

static unsigned int get_scheduling_policy_from_string(const char *sched)
{
        int ret = 0;

        if (!sched) goto out;

        if (!strncmp(sched, "ext", 3)) {
                ret = SCHED_EXT;
        } else if (!strncmp(sched, "ipanema", 7)) {
                ret = SCHED_IPANEMA;
        }
out:
        return ret;
}

void usage(char **argv)
{
        fprintf(stderr,
        "Usage : %s  scheduling_policy ipanema_scheduling_policy programm [args...]\n"
                "\tscheduling_policy            : ext, ipanema\n"
                "\tipanema_scheduling_policy    : 0, 1, 2, ... Used for Ipanema, see /proc/ipanema/policies, even for sched_ext set a placeholder \n"
                "\tprogramm                     : program to run with the specified scheduler\n"
                "\targs                         : arguments for the program\n", argv[0]);      
}

static int sys_sched_setattr(pid_t target, struct sched_attr *attr, unsigned int flags)
{
        return syscall(SYS_sched_setattr, target, attr, flags);
}

