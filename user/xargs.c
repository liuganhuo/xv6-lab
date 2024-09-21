#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[]){
    char *command = argv[1];
    char *arg[MAXARG];
    char buf[512];
    char charbuf[512];
    int cur_argnum = 0;
    int input_size = -1;
    char *p = buf;
    int buf_size = 0;

    int i;
    for (i = 1; i < argc; i++) {
        arg[i - 1] = argv[i];
    }
    cur_argnum = argc - 1;


    while((input_size = read(0, charbuf, sizeof(charbuf))) > 0){
        fprintf(2, "buf : %s",buf);
        for(i = 0; i < input_size; i++){
            if (charbuf[i] == '\n') {
                buf[buf_size] = 0;
                arg[cur_argnum++] = p;
                arg[cur_argnum] = 0;

                if (fork() == 0) {
                    exec(command, arg);
                }
                wait(0);
                
                buf_size = 0;
                cur_argnum = argc - 1;
                p = buf;
            } else if (charbuf[i] == ' ') {
                buf[buf_size++] = 0;
                arg[cur_argnum++] = p;
                p = buf + buf_size;
            } else {
                buf[buf_size++] = charbuf[i];
            }
        }
  }
  exit(0);
}