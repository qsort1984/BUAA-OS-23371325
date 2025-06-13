#include <lib.h>

void usage(void) {
	printf("usage: mkdir [-p] <dir>\n");
	exit();
}

int main(int argc, char *argv[]) {
	int r;
	struct Stat st;
	if (argc < 2) {
		usage();
	}

    if (strcmp(argv[1], "-p") == 0) {
        // 当使用 -p 选项时忽略错误，若目录已存在则直接退出，若创建目录的父目录不存在则递归创建目录。
        if (stat(argv[1], &st) >= 0) {
			return 0;
		}
        // todo
    } else {
        if (stat(argv[1], &st) >= 0) {
			printf("mkdir: cannot create directory '%s': File exists\n", argv[1]);
            return -1;
		}
		if ((r = mkdir(argv[1])) < 0) {
			printf("mkdir: cannot create directory '%s': No such file or directory\n", argv[1]);
            return -1;
		}
    }

	return 0;
}