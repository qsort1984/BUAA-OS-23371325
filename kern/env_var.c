#include <env_var.h>
#include <printf.h>
#include <string.h>

struct EnvVar env_vars[MAX_VARS];
static int envNow = 0;
static int idNow = 1;

int create_shell_id() {
	int ret = idNow;
	++idNow;
	return ret;
}

int declare_env_value(char *name, char *value, int shell_id, int readonly) {
	int i;
	for (i = 0; i < envNow; i++) {
		if (!env_vars[i].in_use) {
			continue;
		}
		if (env_vars[i].shell_id == shell_id || env_vars[i].shell_id == 0) {
			if (strcmp(env_vars[i].name, name) == 0) {
				if (env_vars[i].readonly == 1) {
					return -1;
				} else if (env_vars[i].shell_id == shell_id) {
					env_vars[i].value = value;
					env_vars[i].readonly = readonly;
					return 0;
				}
			}
		}
	}
	strcpy(env_vars[envNow].name, name);
	strcpy(env_vars[envNow].value, value);
	env_vars[envNow].shell_id = shell_id;
	env_vars[envNow].readonly = readonly;
	env_vars[envNow].in_use = 1;
	++envNow;
	return 0;
}

int unset_env_value(char *name, int shell_id) {
	int i;
	for (i = 0; i < envNow; i++) {
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
		}
	} 
	return 0;
}

//type: 0 means find env value named name, 1 means find all env value
//type 0: return: value
//type 0: first consider part to part, then consider part to global
//type 1: print
int get_env_value(char *name, int type, int shell_id) {
	int i;
	if (type == 0) {
		for (i = 0; i < envNow; i++) {
			if (!env_vars[i].in_use) {
				continue;
			}
			if (env_vars[i].shell_id != shell_id) {
				continue;
			}
			if (strcmp(name, env_vars[i].name) == 0) {
				return env_vars[i].value;
			}
		}
		for (i = 0; i < envNow; i++) {
			if (!env_vars[i].in_use) {
				continue;
			}
			if (env_vars[i].shell_id != 0) {
				continue;
			}
			if (strcmp(name, env_vars[i].name) == 0) {
				return env_vars[i].value;
			}
		}
		return -1;
	} else if (type == 1) {
		for (i = 0; i < envNow; i++) {
			if (!env_vars[i].in_use) {
				continue;
			}
			if (env_vars[i].shell_id != shell_id && env_vars[i].shell_id != 0) {
				continue;
			}
			printf("%s=%s\n", env_vars[i].name, env_vars[i].value);
		}
	}
	return 0;
}

