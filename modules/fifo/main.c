#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>


int main(int argc, char **argv)
{
    int pid;

    if ((pid = fork())) {
        printf("Hello c le pere\n");
    } else {
        printf("Hello c le fils\n");
        sleep(3);
        if ((pid = fork())) {
            printf("fils avant petit fils\n");
        } else {
            printf("petit fils\n");
        }
    }

    wait(NULL);
    return 0;
}