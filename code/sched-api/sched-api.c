#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#include "sched-api.h"

int get_scheduling_policy(void)
{
        struct user_sched_attr attr;;
        int ret;

        ret = syscall(SYS_sched_getattr, 0, &attr, sizeof(attr), 0);

        if (ret == -1) {
                perror("sched_getattr failed");
                exit(EXIT_FAILURE);
        }
        
// #define pr_fmt(name, fmt, val) printf("%-14s = " fmt "\n", name, val)
//         pr_fmt("size", "%u", attr.size);
//         pr_fmt("sched_policy", "%u", attr.sched_policy);
//         pr_fmt("sched_flags", "%llu", attr.sched_flags);
//         pr_fmt("sched_nice", "%d", attr.sched_nice);
//         pr_fmt("sched_priority", "%u", attr.sched_priority);


// #undef pr_fmt

        return attr.sched_policy;
}

void set_ipanema_scheduler(void)
{
        // struct user_sched_attr attr = {
	// 	.size = sizeof(struct user_sched_attr),
	// 	.sched_policy = SCHED_IPANEMA,
	// 	.sched_flags = 0,
	// 	.sched_nice = 0,
	// 	.sched_priority = 0,
	// 	.sched_ipa_policy = 0,
	// 	.sched_ipa_attr_size = 0,
	// 	.sched_ipa_attr = NULL,
	// };
        // int ret;

        // ret = syscall(SYS_sched_setattr, 0, &attr, 0);
        // if (ret == -1) {
        //         perror("sched_setattr failed");
        //         exit(EXIT_FAILURE);
        // }

        struct sched_param param = {
                .sched_priority = 0,
        };
        int ret;
        ret = sched_setscheduler(0, SCHED_IPANEMA, &param);
        if (ret == -1) {
                perror("sched_setscheduler failed");
                exit(EXIT_FAILURE);
        }
}

void set_ipanema_scheduler_with_pthread(void)
{
        struct sched_param param = {
                .sched_priority = 0,
        };
        int ret;

        ret = pthread_setschedparam(pthread_self(), SCHED_IPANEMA, &param);
        if (ret != 0) {
                fprintf(stderr, "pthread_setschedparam failed: %s\n", strerror(ret));
                exit(EXIT_FAILURE);
        }
}

void set_eevdf_scheduler()
{
        struct user_sched_attr attr = {
		.size = sizeof(struct user_sched_attr),
		.sched_policy = SCHED_NORMAL,
		.sched_flags = 0,
		.sched_nice = 0,
		.sched_priority = 0,
	};

                int ret;

        ret = syscall(SYS_sched_setattr, 0, &attr, 0);
        if (ret == -1) {
                perror("sched_setattr failed");
                exit(EXIT_FAILURE);
        }

}


void child_process_fn(void)
{

        int policy;
        int exit_code = EXIT_SUCCESS;

        policy = get_scheduling_policy();
        if (policy != SCHED_IPANEMA) {
                fprintf(stderr, "Child process: Wrong policy returned: %d\n", policy);
                exit_code = EXIT_FAILURE;
        }

        printf("Child process has the IPANEMA scheduler set\n");

        exit(exit_code);
}


void *thread_function(void *arg)
{
        struct thread_info *info = (struct thread_info *)arg;
        int policy;
        int exit_code = EXIT_SUCCESS;

        policy = get_scheduling_policy();
        if (policy != SCHED_IPANEMA) {
                fprintf(stderr, "Thread %u: Wrong policy returned: %d\n", info->index, policy);
                exit_code = EXIT_FAILURE;
        }

        printf("Thread %u has the IPANEMA scheduler set\n", info->index);

        pthread_exit((void *)(long)exit_code);
}


struct thread_info *threads_info;

void threads_test(void)
{

        threads_info = malloc(NR_THREADS * sizeof(struct thread_info));
        if (!threads_info) {
                perror("malloc failed");
                exit(EXIT_FAILURE);
        }

        for (unsigned int i = 0; i < NR_THREADS; i++) {
                threads_info[i].index = i;
                if (pthread_create(&threads_info[i].tid, NULL, thread_function, &threads_info[i]) != 0) {
                        perror("pthread_create failed");
                        goto out; // Better error handling, will be killed anyway
                }
        }


        for (unsigned int i = 0; i < NR_THREADS; i++) {
                if (pthread_join(threads_info[i].tid, NULL) != 0) {
                        perror("pthread_join failed");
                        goto out;
                }
        }
out:
        free(threads_info);

}

int thread_policies[NR_THREADS] = {0};


void *thread_setting_sched_fn(void *arg)
{
        struct thread_info *info = (struct thread_info *)arg;
        int with_pthread = info->data;
        int policy;

        /* Only the first thread will change the scheduler */
        if (info->index == 0) {
                printf("Thread %u setting scheduler for himself\n", info->index);

                if (with_pthread)
                        set_ipanema_scheduler_with_pthread();
                else
                        set_ipanema_scheduler();
        } else {
                
                sleep(3);
        }

        thread_policies[info->index] = get_scheduling_policy();
}

void thread_setting_scheduler(unsigned int with_pthread)
{

        threads_info = malloc(NR_THREADS * sizeof(struct thread_info));
        if (!threads_info) {
                perror("malloc failed");
                exit(EXIT_FAILURE);
        }

        for (unsigned int i = 0; i < NR_THREADS; i++) {
                threads_info[i].index = i;
                threads_info[i].data = with_pthread;
                if (pthread_create(&threads_info[i].tid, NULL, thread_setting_sched_fn, &threads_info[i]) != 0) {
                        perror("pthread_create failed");
                        goto out; // Better error handling, will be killed anyway
                }
        }


        for (unsigned int i = 0; i < NR_THREADS; i++) {
                if (pthread_join(threads_info[i].tid, NULL) != 0) {
                        perror("pthread_join failed");
                        goto out;
                }
        }

out:
        free(threads_info);

        for(unsigned int i = 0; i < NR_THREADS; ++i) {
                printf("thread %u has scheduling policy : %d\n", i, thread_policies[i]);
        }
}

int main(int argc, char **argv)
{
        int policy, status;
        pid_t pid;
        printf("Parent (%d) Attr before setting scheduler \n", getpid());
        
        policy = get_scheduling_policy();

        printf("Parent starting policy %d\n", policy);
        set_ipanema_scheduler();
        policy = get_scheduling_policy();

        if (policy != SCHED_IPANEMA) {
                fprintf(stderr, "Wrong policy returend : %d\n", policy);
                exit(EXIT_FAILURE);
        }
        printf("Parrent has set the IPANEMA scheduler\n");



        /*
                Test on processes
        */
        if ((pid = fork()) < 0) {
                perror("fork failed");
                exit(EXIT_FAILURE);
        } else if (pid == 0) {
                child_process_fn();
        }

        wait(&status);

        /* Threads tests */

        threads_test();

        /* Reset scheduling policy */

        set_eevdf_scheduler();
        policy = get_scheduling_policy();
        if (policy != SCHED_NORMAL) {
                fprintf(stderr, "Wrong policy returned after reset: %d\n", policy);
                exit(EXIT_FAILURE);
        }

        printf("Parent has reset the scheduling policy back to SCHED_NORMAL\n");
        printf("Test setting scheduler from thread 0 with sys_sched_setattr\n");
        thread_setting_scheduler(0);

        printf("Test setting scheduler from thread 0 with pthread_setschedparam\n");
        thread_setting_scheduler(1);
}