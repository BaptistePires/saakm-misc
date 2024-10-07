#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>


static int NR_THREADS = 20;

struct thread_info{
        pthread_t id;
        unsigned int index;
        FILE *file;
        int started;
} *threads_info;



void *thread_fn(void *arg)
{

        struct thread_info *ti = (struct thread_info *) arg;

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

        srand(time(NULL));

        threads_info = (struct thread_info *) malloc(NR_THREADS * sizeof(struct thread_info));

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