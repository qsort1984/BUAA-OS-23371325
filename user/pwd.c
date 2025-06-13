#include <lib.h>

int main() {
	if (argc == 1) {
		char path[MAXPATHLEN];
		getcwd(path);
		printf("%s\n", path);
	} else {
		printf("pwd: expected 0 arguments; got %d\n", argc - 1);
	}

	return 0;
}