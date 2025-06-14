#define MAX_VARS 128
#define MAX_NAME_LEN 16
#define MAX_VAL_LEN 16

struct EnvVar {
    char name[MAX_NAME_LEN + 1];
    char value[MAX_VAL_LEN + 1];
    int shell_id; // 0:global
    // int local;  // 是否是局部变量
    int readonly;  // 是否是只读变量
    int in_use;    // 是否占用
};

int create_shell_id(void);

int declare_env_value(char *name, char *value, int shell_id, int readonly);

int unset_env_value(char *name, int shell_id);

int get_env_value(char *name, int type, int shell_id);