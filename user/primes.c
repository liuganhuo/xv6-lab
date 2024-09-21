#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void primes(int read_fd, int write_fd, int max_num){
    char child_visited[max_num];
    read(read_fd, child_visited, max_num);

    int i;
    for (i = 1; i < max_num; i++) {
        if (child_visited[i] == 0) {
            break;
        }
    } 
    if (i < max_num) {
        fprintf(2, "prime %d\n", i);
        // fprintf(2, "num11 %d\n", child_visited[11]);
    } else {
        exit(0);
    }

    for (int j = i; j < max_num; j += i) {
        child_visited[j] = 1;
    }

    int pid = fork();
    if (pid == 0) {
        primes(read_fd, write_fd, max_num);
        // wait(0);
    } else {
        close(read_fd);
        write(write_fd, child_visited, max_num);
        close(write_fd);
        wait(0);
    }
}

int main(){
    int p[2];
    char visited[36] = {0};
    int max_num = 36;

    if (pipe(p) < 0){
        printf("pipe() failed\n");
        exit(1);
    }

    int pid = fork();
    if (pid == 0) {
        primes(p[0], p[1], max_num);
        // wait(0);
    } else {
        close(p[0]);
        visited[0] = 1;
        visited[1] = 1;

        write(p[1], visited, max_num);
        wait(0);
    }
    exit(0);
}