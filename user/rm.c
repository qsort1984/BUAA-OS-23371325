#include <lib.h>

void usage(void) {
	printf("usage: rm <file> | rm [-r|-rf] <file>|<dir>\n");
	exit();
}

int rm(char *path, int mode) {
    struct Stat st;
    if (stat(path, &st) < 0) {
        if (mode) {
            printf("rm: cannot remove '%s': No such file or directory\n", path);
            return -1;
        } else {
            return 0;
        }
    }

    try(remove(path));

    return 0;
}

int main(int argc, char *argv[]) {
    struct Stat st;

	if (argc < 2) {
		usage();
	}

    if (strcmp(argv[1], "-r") == 0) {
        return rm(argv[2], 1);
    } else if (strcmp(argv[1], "-rf") == 0){
        return rm(argv[2], 0);
    } else {
        if (stat(argv[1], &st) < 0) {
            printf("rm: cannot remove '%s': No such file or directory\n", argv[1]);
            return -1;
        }
        if (st.st_isdir) {
            printf("rm: cannot remove '%s': Is a directory\n", argv[1]);
            return -1;
        }
        try(remove(argv[1]));
    }

	return 0;
}