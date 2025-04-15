#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sched.h>
#include <unistd.h>

static int NR_THREADS = 20;
struct thread_info {
	pthread_t id;
	unsigned int index;
	FILE *file;
	int started;
} *threads_info;

void *thread_fn(void *arg)
{
	struct thread_info *ti = (struct thread_info *)arg;
	struct sched_param param = { .sched_priority = 0 };
	int ret;
	if (ti->index == 10) {
		if ((ret = pthread_setschedparam(pthread_self(), 0x3,
						 &param)) == -1) {
			printf("error setting sched :) : %s \n", strerror(ret));
		} else {
			usleep(1000000);
		}
	}
	printf("[T%u] Hello, world!\n", ti->index);

	for (unsigned int i = 0; i < 100; i++) {
		usleep(rand() % 20000);
		fprintf(ti->file, "%u", i + rand());
	}
	return NULL;
}

int main(int argc, char **argv)
{
	struct thread_info *curr;
	struct sched_param param = { .sched_priority = 0 };
	srand(time(NULL));

	/* Set sched */

	threads_info = (struct thread_info *)malloc(NR_THREADS *
						    sizeof(struct thread_info));
	if (!threads_info) {
		perror("malloc");
		return EXIT_FAILURE;
	}

	for (unsigned int i = 0; i < NR_THREADS; ++i) {
		curr = &threads_info[i];
		curr->index = i;
		curr->file = tmpfile();
		if (!curr->file) {
			perror("tmpfile");
			continue;
		}
		pthread_create(&curr->id, NULL, thread_fn, curr);
		curr->started = 1;
	}

	for (unsigned int i = 0; i < NR_THREADS; ++i) {
		curr = &threads_info[i];
		if (curr->started) {
			pthread_join(curr->id, NULL);
			fclose(curr->file);
		}
	}
	return 0;
}
