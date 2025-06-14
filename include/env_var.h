#define MAX_VARS 128
#define MAX_NAME_LEN 16
#define MAX_VAL_LEN 16

struct EnvVar {
    char name[MAX_NAME_LEN + 1];
    char value[MAX_VAL_LEN + 1];
    int shell_id; // 0 -> 全局环境变量 others -> 局部环境变量
    int readonly;  // 是否是只读变量
    int in_use;    // 是否占用
};

int alloc_shell_id();

int declare_env_var(char *name, char *value, int shell_id, int readonly);

int unset_env_var(char *name, int shell_id);

char *get_env_var(char *name, int shell_id);

int print_vars(int shell_id);