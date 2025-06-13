#include <lib.h>

int main(int argc, char **argv) {
	struct Stat st;
    char *path;

    if (argc == 1) {
        // 只有 "cd"，切换到根目录
        path = "/";
    } else if (argc == 2) {
        path = argv[1];

        if (strcmp(path, ".") == 0) {
            // 解析为当前目录 /dir1/dir2，无变化
            return 0;
        } else if (strcmp(path, "..") == 0) {
            // 解析为 /dir1，若存在且为目录，切换到该目录
            // todo
            return 0;
        }

        if (stat(path, &st) < 0) {
            printf("cd: The directory '%s' does not exist\n", argv[1]);
            return 1;
        }

        if (!st.st_isdir) {
            printf("cd: '%s' is not a directory\n", argv[1]);
            return 1;
        }

        try(chdir(path));

        return 0;
    } else {
        printf("Too many args for cd command\n");
        return 1;
    }
}