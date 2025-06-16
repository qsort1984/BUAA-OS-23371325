#include <args.h>
#include <lib.h>

#define WHITESPACE " \t\r\n"
#define SYMBOLS "<|>&;()"
#define MAXARGS 128

u_int shell_envid; // 当前 shell 对应的进程 id
int shell_id; // 当前 shell 对应的环境变量页面 id

#define MAX_NAME_LEN 16
#define MAX_VAL_LEN 16
#define MAX_ARGV_LEN 1024
#define HISTORY_SIZE 20

char buffer[MAXARGS + 1][MAX_VAL_LEN + 1];
char history_buf[HISTORY_SIZE][MAX_ARGV_LEN];
int history_valid[HISTORY_SIZE];
int history_index = 0;
int current_index = 0;

int runcmd(char *s);

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

	// 识别反引号括起的内容
	// 反引号也要被视为一部分
	if (*s == '`') {
		*p1 = s;
		s++;
		while (*s && *s != '`') {
			s++;
		}
		if (*s == '`') {
			s++;
		}
		*p2 = s;
		return 'w';
	}


	// 识别 || 和 &&
	if ((*s == '&' && *(s + 1) == '&') || (*s == '|' && *(s + 1) == '|')) {
        int t = (*s == '&') ? 300 : 301;
        *s++ = 0;
        *s++ = 0;
        *p2 = s;
        return t;
    }

	if (*s == '>' && *(s + 1) == '>') {
        *s++ = 0;
        *s++ = 0;
        *p2 = s;
        return 302;
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

int isWhite(char c) {
	return c == '\n' || c == '\r' || c == ' ' || c == '\t';
}

int run_command_and_capture_output(const char *cmd, char *output) {
    int p[2];
	int r = pipe(p);
    if (r != 0) {
        debugf("pipe: %d\n", r);
		exit();
    }

	r = fork();
	if (r < 0) {
		debugf("fork: %d\n", r);
		exit();
	}

	if (r == 0) {
		// 子进程
		close(p[0]);  // 关闭读端
        dup(p[1], 1); // stdout -> pipe写端
        runcmd(cmd);
		close(p[1]);
        exit();
	} else {
		// 父进程
		close(p[1]);  // 关闭写端
		int n = read(p[0], output, MAX_ARGV_LEN - 1);
		if (n >= 0) {
			for (int i = 0; i < n; i++) {
				if (output[i] == '\n' || output[i] == '\r' || 
					(isWhite(output[i]) && isWhite(output[i + 1]))) {
					output[i] = '\0';
					break;
				}
			}
			// for (int i = 0; i < n; i++) {
			// 	if (output[i] == '\n' || output[i] == '\r' || output == ' ' || output == '\t') {
			// 		output[i] = '\0';
			// 		break;
			// 	}
			// }
			output[n] = '\0';
		} else {
			output[0] = '\0';
		}
		// printf("output is %s and is here\n", output);
		close(p[0]);
		wait(r);
	}

    return 0;
}

int expand_var(const char *buffer, const char *word) {
	char *buffer_copy = buffer;
	while (*word) {
		if (*word == '$') {
			int len = strlen(word);
			char tmp[len + 1];
			word++;
			int i = 0;
			while (*word && *word != '/') { // 判断条件应该为 *word 时标识符的组成部分 todo
				tmp[i++] = *word++;
			}
			tmp[i] = '\0';
			try(syscall_get_env_var(buffer_copy, tmp, shell_id));
			buffer_copy += strlen(buffer_copy);
		} else if (*word == '`') {
			char tmp[MAX_ARGV_LEN];
			char *q = tmp;
			word++;
			while (*word && *word != '`') {
				*q++ = *word++;
			}
			if (*word == '`') {
				word++;
			}
			*q = '\0';
			try(run_command_and_capture_output(tmp, buffer_copy));
			// printf("buffer_copy is %s is here\n", buffer_copy);
			buffer_copy += strlen(buffer_copy);
		} else {
			*buffer_copy++ = *word++;
		}
	}
	*buffer_copy = '\0';

	return 0;
}

int child_tag = 0; // 为 1 时表示为子进程
int lazy = 0; // 懒位，不为 0 时不执行后续指令 1 -> && -1 -> ||

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
			buffer[argc][0] = '\0';
			try(expand_var(buffer[argc], t));
			argv[argc] = buffer[argc];
			argc++;
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
		case '|':
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
			break;
		case ';':
			// 新建子进程执行;前的指令，父进程等待子进程执行完毕
			if ((r = fork()) < 0) { 
				debugf("fork: %d\n", r);
				exit();
			}
			if (r == 0) {
				return argc;
			} else {
				wait(r);
				return parsecmd(argv, rightpipe);
			}
			break;
		case 300:
			// &&
			if ((r = fork()) < 0) { 
				debugf("fork: %d\n", r);
				exit();
			}
			if (r == 0) {
				child_tag = 1;
				return argc;
			} else {
				child_tag = 0;
				int result = ipc_recv(NULL, 0, 0);
				// if (*rightpipe == 0){
				// 	dup(1, 0);
				// } else if (*rightpipe == 1) {
				// 	dup(0, 1);
				// }
				wait(r);
				if (result != 0) {
					lazy = 1;
				} else {
					lazy = 0;
				}
				return parsecmd(argv, rightpipe);
			}
			break;
		case 301:
			// ||
			if ((r = fork()) < 0) { 
				debugf("fork: %d\n", r);
				exit();
			}
			if (r == 0) {
				child_tag = 1;
				return argc;
			} else {
				int result = ipc_recv(NULL, 0, 0);
				child_tag = 0;
				// if (*rightpipe == 0){
				// 	dup(1, 0);
				// } else if (*rightpipe == 1) {
				// 	dup(0, 1);
				// }
				wait(r);
				if (result == 0) {
					lazy = -1;
				} else {
					lazy = 0;
				}
				return parsecmd(argv, rightpipe);
			}
			break;
		case 302:
			// >>
			if (gettoken(0, &t) != 'w') {
				debugf("syntax error: > not followed by word\n");
				exit();
			}
			fd = open(t, O_WRONLY | O_CREAT);
			if (fd < 0) {
				debugf("failed to open '%s'\n", t);
				exit();
			}
			struct Fd *fd_struct = (struct Fd*) num2fd(fd);
			struct Filefd *ffd = (struct Filefd*) fd_struct;
			fd_struct->fd_offset = ffd->f_file.f_size;
			dup(fd, 1);
			close(fd);
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

int history(int argc, char **argv) {
	if (argc == 1) {
		argv[0] = "/cat.b";
		argv[1] = "/.mos_history";
	} else {
		debugf("history: expected 0 arguments; got %d\n", argc - 1);
		return 2;
	}

	return 0;
}

int runcmd(char *s) {
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
	} else if (strcmp(argv[0], "declare") == 0) {
		return declare(argc, argv);
	} else if (strcmp(argv[0], "unset") == 0) {
		return unset(argc, argv);
	} else if (strcmp(argv[0], "history") == 0) {
		try(history(argc, argv));
	}

	// 条件执行
	if (lazy != 0) {
		if (lazy == 1) { // &&
			if (child_tag) {
				ipc_send(syscall_get_parent_id(), 1, NULL, 0);
			}
		} else { // ||
			if (child_tag) {
				ipc_send(syscall_get_parent_id(), 0, NULL, 0);
			}
		}
		lazy = 0;
		exit();
	}

	int child = spawn(argv[0], argv);
	int res = ipc_recv(NULL, 0, 0);
	close_all();
	if (child >= 0) {
		// if (child_tag == 1) {
		// 	ipc_send(syscall_get_parent_id(), res, NULL, 0);
		// }
		wait(child);
	} else {
		debugf("spawn %s: %d\n", argv[0], child);
	}
	if (rightpipe) {
		wait(rightpipe);
	}
	exit();
}

void savecmd(char *buf) {
	strcpy(history_buf[current_index], buf);
	history_valid[current_index] = 1;
}

void readline(char *buf, u_int n) {
	int r;
	int pointer = 0; // 指向光标所在位置
	int max = 0; // 最大字符数
	char c; // 从标准输入中读取的字符
	for (int i = 0; i < n; i++) {
		if ((r = read(0, &c, 1)) != 1) {
			if (r < 0) {
				debugf("read error: %d\n", r);
			}
			exit();
		}
		if (c == '\b' || c == 0x7f) {
			// backspace : 删除光标左侧 1个字符并将光标向左移动 1列；若已在行首则无动作
			if (pointer != 0) {
				for (int j = pointer - 1; j < max - 1; j++) {
					buf[j] = buf[j + 1];
				}
				max--;
				pointer--;
                printf("\b"); 

				// 重写后续字符覆盖原字符
				for (int j = pointer; j < max; j++) {
					printf("%c", buf[j]);
				}
				printf(" ");

				// 光标右移回去
				for (int j = max; j > pointer; j--) {
					printf("\b");
				}
				printf("\b");
				i -= 2;
			} else {
				i--;
			}
			continue;
		}
		// 处理上下左右键
		if (c == 27) {
            r = read(0, &c, 1);
			if (c != 91) {
				debugf("unkonwn char: %c\n", c);
			}
            r = read(0, &c, 1);
            if (c == 65) { // up
				printf("%c%c%c", 27, 91, 66); // 下移一个抵消输入
				buf[max] = '\0';
				savecmd(buf);
				if (history_valid[(current_index + HISTORY_SIZE - 1) % HISTORY_SIZE] && current_index != ((history_index + 1) % HISTORY_SIZE)) {
					// 移动光标
					for (int j = 0; j < pointer; j++) {
						printf("\b");
					}
					current_index = (current_index + HISTORY_SIZE - 1) % HISTORY_SIZE;
					strcpy(buf, history_buf[current_index]);
					printf("%s", buf);
					for (int j = strlen(buf); j < max; j++) {
						printf(" ");
					}
					for (int j = strlen(buf); j < max; j++) {
						printf("\b");
					}
					i = max = pointer = strlen(buf);
				}
            } else if (c == 66) { // down
				// printf("%c%c%c", 27, 91, 65); 不需上移
				buf[max] = '\0';
				savecmd(buf);
                if (history_valid[(current_index + 1) % HISTORY_SIZE] && history_index != current_index) {
					// 移动光标
					for (int j = 0; j < pointer; j++) {
						printf("\b");
					}
					current_index = (current_index + 1) % HISTORY_SIZE;
					strcpy(buf, history_buf[current_index]);
					printf("%s", buf);
					for (int j = strlen(buf); j < max; j++) {
						printf(" ");
					}
					for (int j = strlen(buf); j < max; j++) {
						printf("\b");
					}
					i = max = pointer = strlen(buf);
				}
            } else if (c == 67) { // right
                if (pointer < max) {
					pointer++;
                } else {
					printf("\b"); // 回退一格抵消输入
				}
				i--;
            } else if (c == 68) { // left
                if (pointer > 0) {
					pointer--;
                } else {
					printf(" "); // 前进一格抵消输入
				}
				i--;
            } else {
				debugf("unkonwn char: %c\n", buf[i]);
			}
            continue;
		}
		// 处理快捷键
		if (c == 1) {  // Ctrl-A
			while (pointer > 0) {
				printf("\b");
				pointer--;
			}
			i--;
			continue;
		} else if (c == 5) {  // Ctrl-E
			while (pointer < max) {
				printf("%c", buf[pointer]);
				pointer++;
			}
			i--;
			continue;
		} else if (c == 11) {  // Ctrl-K
			for (int j = pointer; j < max; j++) {
				printf(" ");
			}
			for (int j = pointer; j < max; j++) {
				printf("\b");
			}
			max = pointer;
			i = pointer - 1;
			continue;
		} else if (c == 21) {  // Ctrl-U
			// 删除从行首到光标前
			while (pointer > 0) {
				for (int j = 0; j < max - 1; j++) {
					buf[j] = buf[j + 1];
				}
				max--;
				pointer--;
				printf("\b");
				for (int j = pointer; j < max; j++) {
					printf("%c", buf[j]);
				}
				printf(" ");
				for (int j = pointer; j <= max; j++) {
					printf("\b");
				}
			}
			i = max - 1;
			continue;
		} else if (c == 23) {  // Ctrl-W
			// Step 1: 跳过左侧空格
			int cnt = 0;
			while (pointer > 0 && (buf[pointer - 1] == ' ' || buf[pointer - 1] == '\t')) {
				for (int j = pointer - 1; j < max - 1; j++) {
					buf[j] = buf[j + 1];
				}
				pointer--;
				max--;
				printf("\b");
				for (int j = pointer; j < max; j++) {
					printf("%c", buf[j]);
				}
				printf(" ");
				for (int j = pointer; j <= max; j++) {
					printf("\b");
				}
				cnt++;
			}

			// Step 2: 删除连续非空白字符
			while (pointer > 0 && buf[pointer - 1] != ' ' && buf[pointer - 1] != '\t') {
				for (int j = pointer - 1; j < max - 1; j++) {
					buf[j] = buf[j + 1];
				}
				pointer--;
				max--;
				printf("\b");
				for (int j = pointer; j < max; j++) {
					printf("%c", buf[j]);
				}
				printf(" ");
				for (int j = pointer; j <= max; j++) {
					printf("\b");
				}
				cnt++;
			}

			i -= cnt + 1; // to change
			continue;
		}

		if (c == '\r' || c == '\n') {
			buf[max] = 0;
			return;
		}
		// 写入普通字符
		// 更新显示界面
		for (int j = pointer; j < max; j++) {
			printf("%c", buf[j]);
		}
		for (int j = pointer; j < max; j++) {
			printf("\b");
		}
		// 更新 buf
		for (int j = max; j > pointer; j--) {
			buf[j] = buf[j - 1];
		}
		buf[pointer] = c;
		pointer++;
		max++;
	}
	debugf("line too long\n");
	// 读取完剩余的字符
	while ((r = read(0, &c, 1)) == 1 && c != '\r' && c != '\n') {
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
	if ((r = open("/.mos_history", O_CREAT)) < 0) {
		user_panic("open /.mos_history: %d", r);
	}
	close(r);
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
		history_valid[history_index] = 0;
		current_index = history_index;

		// 把命令写入history
		if (strlen(buf) > 0) {
			savecmd(buf);
			history_index = (history_index + 1) % HISTORY_SIZE;
			current_index = history_index;
		}
		if ((r = open("/.mos_history", O_RDWR)) < 0) {
			user_panic("open /.mos_history: %d", r);
		} else {
			for (int i = 0; i < HISTORY_SIZE; i++) {
				char *tmp = history_buf[(history_index + i) % HISTORY_SIZE];
				if (history_valid[(history_index + i) % HISTORY_SIZE]) {
					fprintf(r, "%s\n", tmp);
				}
			}
			close(r);
		}

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

		if (strcmp(buf, "exit") == 0) {
			// 条件判断不够严谨
			break;  // 退出主 shell
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
