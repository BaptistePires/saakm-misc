#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>


void subchild_fn(void)
{
        int res = rand();
        for (unsigned int j = 0; j < (rand() % 100000); ++j) {
                res += rand();
        }

        fprintf(stderr, "%d", res);
}

void child_fn(void)
{
        pid_t pid;
        int res = rand();
        int nr_children = 2;
        for (unsigned int j = 0; j < nr_children; ++j) {
                if ((pid = fork())) {
                        subchild_fn();
                        return;
                }
        }

        for (unsigned int j = 0; j < nr_children; ++j) {
                wait(NULL);
        }

}

int main(int argc, char** argv)
{

        int nr_children = 100;
        pid_t pid;
        
        srand(time(NULL));

        
        for (unsigned int i = 0; i < nr_children; ++i) {
                if ((pid = fork()) == 0) {
                       child_fn();
                        return;
                }
        }

        for (unsigned int i = 0; i < nr_children; ++i) {
                wait(NULL);
        }
        
        return 0;
}
