#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]){
    int p1[2];
    int p2[2];
    if (pipe(p1) < 0){
        printf("pipe() failed\n");
        exit(1);
    }
    if (pipe(p2) < 0){
        printf("pipe() failed\n");
        exit(1);
    }


    int pid = fork();
    if (pid == 0) {
        int child_pid = getpid();

        char child_c;
        close(p1[0]);
        close(p2[1]);
        read(p2[0], &child_c, 1);
        fprintf(2, "%d: received ping\n", child_pid);
        write(p1[1], &child_c, 1);
        exit(0);
    } else {
        int parent_pid = getpid();
        char c;
        char parent_c;
        c = 'A';

        close(p1[1]);
        close(p2[0]);
        write(p2[1], &c, 1);
        read(p1[0], &parent_c, 1);
        fprintf(2, "%d: received pong\n", parent_pid);
        if (c != parent_c) {
            fprintf(2, "error in ping pong\n");
        }
        exit(0);
    }
}