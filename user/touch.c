#include <lib.h>

void usage(void) {
	printf("usage: touch <file>\n");
	exit();
}

int main(int argc, char *argv[]) {
	int fd;
    struct Stat st;
	if (argc < 2) {
		usage();
	}

    if (stat(argv[1], &st) >= 0) {
		return 0;
	}
	
	if ((fd = open(argv[1], O_CREAT)) >= 0) {
		close(fd);
	} else {
        printf("touch: cannot touch '%s': No such file or directory", argv[1]);
    }
	
	return 0;
}