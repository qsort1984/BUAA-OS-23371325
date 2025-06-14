#include <env_var.h>
#include <printf.h>
#include <string.h>

struct EnvVar env_vars[MAX_VARS];
static int vars_num = 0;
static int idNow = 1;

int alloc_shell_id() {
	return idNow++;
}

int declare_env_var(char *name, char *value, int shell_id, int readonly) {
	int i;
	for (i = 0; i < vars_num; i++) {
		if (!env_vars[i].in_use) {
			continue;
		}
		if (env_vars[i].shell_id == shell_id) {
			if (strcmp(env_vars[i].name, name) == 0) {
				if (env_vars[i].readonly) {
					return -1;
				} else {
					strcpy(env_vars[i].value, value);
					env_vars[i].readonly = readonly;
					return 0;
				}
			}
		}
	}
	// 未找到环境变量则需要重新声明
	strcpy(env_vars[vars_num].name, name);
	strcpy(env_vars[vars_num].value, value);
	env_vars[vars_num].shell_id = shell_id;
	env_vars[vars_num].readonly = readonly;
	env_vars[vars_num].in_use = 1;
	vars_num++;
	return 0;
}

int unset_env_var(char *name, int shell_id) {
	int i;
	for (i = 0; i < vars_num; i++) {
		if (!env_vars[i].in_use) {
			continue;
		}
		if (env_vars[i].shell_id != shell_id && env_vars[i].shell_id != 0) {
			continue;
		}
		if (strcmp(env_vars[i].name, name) == 0) {
			if (env_vars[i].readonly == 1) {
				return -1;
			}
			env_vars[i].in_use = 0;
			return 0;
		}
	} 
	return -1;
}

char *get_env_var(char *name, int shell_id) {
	int i;
	for (i = 0; i < vars_num; i++) {
		if (!env_vars[i].in_use) {
			continue;
		}
		if (env_vars[i].shell_id != shell_id && env_vars[i].shell_id != 0) {
			continue;
		}
		if (strcmp(name, env_vars[i].name) == 0) {
			return env_vars[i].value;
		}
	}

	return name;
}

int print_vars(int shell_id) {
	for (i = 0; i < vars_num; i++) {
		if (!env_vars[i].in_use) {
			continue;
		}
		if (env_vars[i].shell_id != shell_id && env_vars[i].shell_id != 0) {
			continue;
		}
		printf("%s=%s\n", env_vars[i].name, env_vars[i].value);
	}

	return 0;
}

