#include <args.h>
#include <lib.h>

#define WHITESPACE " \t\r\n"
#define SYMBOLS "<|>&;()"

u_int shell_envid; // 当前 shell 对应的进程 id
int shell_id; // 当前 shell 对应的环境变量页面 id

#define MAX_NAME_LEN 16
#define MAX_VAL_LEN 16

/* Overview:
 *   Parse the next token from the string at s.
 *
 * Post-Condition:
 *   Set '*p1' to the beginning of the token and '*p2' to just past the token.
 *   Return:
 *     - 0 if the end of string is reached.
 *     - '<' for < (stdin redirection).
 *     - '>' for > (stdout redirection).
 *     - '|' for | (pipe).
 *     - 'w' for a word (command, argument, or file name).
 *
 *   The buffer is modified to turn the spaces after words into zero bytes ('\0'), so that the
 *   returned token is a null-terminated string.
 */
int _gettoken(char *s, char **p1, char **p2) {
	*p1 = 0;
	*p2 = 0;
	if (s == 0) {
		return 0;
	}

	while (strchr(WHITESPACE, *s)) {
		*s++ = 0;
	}
	if (*s == 0) {
		return 0;
	}

	if (strchr(SYMBOLS, *s)) {
		int t = *s;
		*p1 = s;
		*s++ = 0;
		*p2 = s;
		return t;
	}

	*p1 = s;
	while (*s && !strchr(WHITESPACE SYMBOLS, *s)) {
		s++;
	}
	*p2 = s;
	//* 将 $NAME 也视为单词
	return 'w';
}

int gettoken(char *s, char **p1) {
	static int c, nc;
	static char *np1, *np2;

	if (s) {
		nc = _gettoken(s, &np1, &np2);
		return 0;
	}
	c = nc;
	*p1 = np1;
	nc = _gettoken(np2, &np1, &np2);
	return c;
}

int expand_var(char *ret, const char *word) {
	if (*word == '$') {
		try(syscall_get_env_var(ret, word + 1, shell_id));
	} else {
		ret = word;
	}

	return 0;
}

#define MAXARGS 128

int parsecmd(char **argv, int *rightpipe) {
	int argc = 0;
	while (1) {
		char *t;
		int fd, r;
		int c = gettoken(0, &t);
		switch (c) {
		case 0:
			return argc;
		case 'w':
			if (argc >= MAXARGS) {
				debugf("too many arguments\n");
				exit();
			}
			// try(expand_var(argv[argc++], t));
			argv[argc++] = t;
			break;
		case '<':
			if (gettoken(0, &t) != 'w') {
				debugf("syntax error: < not followed by word\n");
				exit();
			}
			// Open 't' for reading, dup it onto fd 0, and then close the original fd.
			// If the 'open' function encounters an error,
			// utilize 'debugf' to print relevant messages,
			// and subsequently terminate the process using 'exit'.
			/* Exercise 6.5: Your code here. (1/3) */
			fd = open(t, O_RDONLY);
			if (fd < 0) {
				debugf("failed to open '%s'\n", t);
				exit();
			}
			dup(fd, 0);
			close(fd);

			// user_panic("< redirection not implemented");

			break;
		case '>':
			if (gettoken(0, &t) != 'w') {
				debugf("syntax error: > not followed by word\n");
				exit();
			}
			// Open 't' for writing, create it if not exist and trunc it if exist, dup
			// it onto fd 1, and then close the original fd.
			// If the 'open' function encounters an error,
			// utilize 'debugf' to print relevant messages,
			// and subsequently terminate the process using 'exit'.
			/* Exercise 6.5: Your code here. (2/3) */
			fd = open(t, O_WRONLY | O_CREAT | O_TRUNC);
			if (fd < 0) {
				debugf("failed to open '%s'\n", t);
				exit();
			}
			dup(fd, 1);
			close(fd);

			// user_panic("> redirection not implemented");

			break;
		case '|':;
			/*
			 * First, allocate a pipe.
			 * Then fork, set '*rightpipe' to the returned child envid or zero.
			 * The child runs the right side of the pipe:
			 * - dup the read end of the pipe onto 0
			 * - close the read end of the pipe
			 * - close the write end of the pipe
			 * - and 'return parsecmd(argv, rightpipe)' again, to parse the rest of the
			 *   command line.
			 * The parent runs the left side of the pipe:
			 * - dup the write end of the pipe onto 1
			 * - close the write end of the pipe
			 * - close the read end of the pipe
			 * - and 'return argc', to execute the left of the pipeline.
			 */
			int p[2];
			/* Exercise 6.5: Your code here. (3/3) */
			r = pipe(p);
			if (r != 0) {
				debugf("pipe: %d\n", r);
				exit();
			}
			r = fork();
			if (r < 0) {
				debugf("fork: %d\n", r);
				exit();
			}
			*rightpipe = r;
			if (r == 0) {
				dup(p[0], 0);
				close(p[0]);
				close(p[1]);
				return parsecmd(argv, rightpipe);
			} else {
				dup(p[1], 1);
				close(p[1]);
				close(p[0]);
				return argc;
			}

			// user_panic("| not implemented");

			break;
		}
	}

	return argc;
}

int cd(int argc, char *argv[]) {
	struct Stat st;
	char *path;

	if (argc == 1) {
		// 只有 "cd"，切换到根目录
		path = "/";
	} else if (argc == 2) {
		path = argv[1];

		if (stat(path, &st) < 0) {
			debugf("cd: The directory '%s' does not exist\n", argv[1]);
			return 1;
		}
		if (!st.st_isdir) {
			debugf("cd: '%s' is not a directory\n", argv[1]);
			return 1;
		}
	} else {
		debugf("Too many args for cd command\n");
		return 1;
	}

	try(chdir(shell_envid, path));
	return 0;
}

int pwd(int argc) {
	if (argc == 1) {
		char path[MAXPATHLEN];
		getcwd(path);
		printf("%s\n", path);
	} else {
		debugf("pwd: expected 0 arguments; got %d\n", argc - 1);
		return 2;
	}

	return 0;
}

void get_name_val(char *src, char *name, char *value) {
	char *p = src;
	while (*p && *p != '=') {
		*name++ = *p++;
	}
	*name = '\0';
	if (*p) {
		p++;
		while (*p) {
			*value++ = *p++;
		}
		*value = '\0'; 
	} else {
		*value = '\0';
	}
}

int declare(int argc, char *argv[]) {
	if (argc == 1) {
		// 输出当前 shell 的所有变量
		try(syscall_print_vars(shell_id));
	} else {
		char name[MAX_NAME_LEN + 1], value[MAX_VAL_LEN + 1];
		if (strcmp(argv[1], "-x") == 0) {
			get_name_val(argv[2], name, value);
			try(syscall_declare_env_var(name, value, 0, 0));
		} else if (strcmp(argv[1], "-r") == 0) {
			get_name_val(argv[2], name, value);
			try(syscall_declare_env_var(name, value, shell_id, 1));
		} else if (strcmp(argv[1], "-xr") == 0) {
			get_name_val(argv[2], name, value);
			try(syscall_declare_env_var(name, value, 0, 1));
		} else {
			get_name_val(argv[1], name, value);
			try(syscall_declare_env_var(name, value, shell_id, 0));
		}
	}

	return 0;
}

int unset(int argc, char *argv[]) {
	if (argc == 1) {
		printf("usage: unset NAME\n");
		return 0;
	} else {
		try(syscall_unset_env_var(argv[1], shell_id));
	}

	return 0;
}

void runcmd(char *s) {
	gettoken(s, 0);

	char *argv[MAXARGS];
	int rightpipe = 0;
	int argc = parsecmd(argv, &rightpipe);
	if (argc == 0) {
		return;
	}
	argv[argc] = 0;

	// 处理内建指令
	if (strcmp(argv[0], "cd") == 0) {
		return cd(argc, argv);
	} else if (strcmp(argv[0], "pwd") == 0) {
		return pwd(argc);
	} else if (strcmp(argv[0], "exit") == 0) {
		exit();
	} else if (strcmp(argv[0], "declare") == 0) {
		return declare(argc, argv);
	} else if (strcmp(argv[0], "unset") == 0) {
		return unset(argc, argv);
	}

	int child = spawn(argv[0], argv);
	close_all();
	if (child >= 0) {
		wait(child);
	} else {
		debugf("spawn %s: %d\n", argv[0], child);
	}
	if (rightpipe) {
		wait(rightpipe);
	}
	exit();
}

void readline(char *buf, u_int n) {
	int r;
	for (int i = 0; i < n; i++) {
		if ((r = read(0, buf + i, 1)) != 1) {
			if (r < 0) {
				debugf("read error: %d\n", r);
			}
			exit();
		}
		if (buf[i] == '\b' || buf[i] == 0x7f) {
			if (i > 0) {
				i -= 2;
			} else {
				i = -1;
			}
			if (buf[i] != '\b') {
				printf("\b");
			}
		}
		if (buf[i] == '\r' || buf[i] == '\n') {
			buf[i] = 0;
			return;
		}
	}
	debugf("line too long\n");
	while ((r = read(0, buf, 1)) == 1 && buf[0] != '\r' && buf[0] != '\n') {
		;
	}
	buf[0] = 0;
}

char buf[1024];

void usage(void) {
	printf("usage: sh [-ix] [script-file]\n");
	exit();
}

int main(int argc, char **argv) {
	int r;
	int interactive = iscons(0);
	int echocmds = 0;
	shell_envid = syscall_getenvid();
	shell_id = syscall_shell_id_alloc();
	printf("\n:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
	printf("::                                                         ::\n");
	printf("::                     MOS Shell 2024                      ::\n");
	printf("::                                                         ::\n");
	printf(":::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
	ARGBEGIN {
	case 'i':
		interactive = 1;
		break;
	case 'x':
		echocmds = 1;
		break;
	default:
		usage();
	}
	ARGEND

	if (argc > 1) {
		usage();
	}
	if (argc == 1) {
		close(0);
		if ((r = open(argv[0], O_RDONLY)) < 0) {
			user_panic("open %s: %d", argv[0], r);
		}
		user_assert(r == 0);
	}
	for (;;) {
		if (interactive) {
			printf("\n$ ");
		}
		readline(buf, sizeof buf);

		// 忽略注释
		if (buf[0] == '#') {
			continue;
		}
		for (int i = 0; i < strlen(buf); i++) {
			if (buf[i] == '#') {
				buf[i] = '\0';
				break;
			}
		}
		if (echocmds) {
			printf("# %s\n", buf);
		}
		if ((r = fork()) < 0) {
			user_panic("fork: %d", r);
		}
		if (r == 0) {
			runcmd(buf);
			exit();
		} else {
			wait(r);
		}
	}
	return 0;
}
