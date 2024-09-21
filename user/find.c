#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

char *fmtname(char *path){
  static char buf[32];
  char *p;

  // Find first character after last slash.
  int cnt = 0;
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    cnt++;
  p++;

  memmove(buf, p, strlen(p));
  buf[strlen(p)] = 0;
  return buf;
}

void find(char *path, char *find_item){
    int fd;
    struct stat st;
    struct dirent de;
    char new_path[512];

    if ((fd = open(path, O_RDONLY)) < 0) {
        // fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch(st.type) {
        case T_DEVICE:
        case T_FILE:
            // if (strcmp(fmtname(path), find_item) == 0) {
            //     fprintf(2, "%s\n", path);
            // }
            break;
        case T_DIR:
            while (read(fd, &de, sizeof(de)) == sizeof(de)) {
                if (de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
                    continue;
                }

                memmove(new_path, path, strlen(path));
                int len = strlen(path);
                if (new_path[len - 1] != '/') {
                    new_path[len] = '/';
                    new_path[len + 1] = '\0';
                    len++;
                } else {
                    new_path[len] = '\0';
                }

                if (strcmp(de.name, find_item) == 0) {
                    printf("%s%s\n", new_path, find_item);
                    continue;
                }

                int all_len = len + strlen(de.name);
                memmove(new_path + len, de.name, strlen(de.name));
                new_path[all_len] = '\0';
                find(new_path, find_item);
            }
            break;
    }
    close(fd);

}


int main(int argc, char *argv[]){
    if (argc != 3) {
        fprintf(2, "error in args num of find\n");
        exit(1);
    }

    char* path = argv[1];
    int path_len = strlen(path);
    char new_path[512];
    char* find_item = argv[2];

    if (path[path_len - 1] != '/') {
        memmove(new_path, path, path_len);
        new_path[path_len] = '/';
        new_path[path_len + 1] = '\0';
    } else {
        memmove(new_path, path, path_len);
    }
    find(new_path, find_item);
    exit(0);
}