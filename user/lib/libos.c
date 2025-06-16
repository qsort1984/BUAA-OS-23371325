#include <env.h>
#include <lib.h>
#include <mmu.h>

int exit_status = -1;

void exit(void) {
	// After fs is ready (lab5), all our open files should be closed before dying.
#if !defined(LAB) || LAB >= 5
	close_all();
#endif

	// syscall_ipc_try_send(env->env_parent_id, exit_status, 0, 0);
	// if (envs[ENVX(env->env_parent_id)].env_ipc_recving != 0) {
    //     ipc_send(env->env_parent_id, exit_status, 0, 0);
    // }
	syscall_env_destroy(0);
	user_panic("unreachable code");
}

const volatile struct Env *env;
extern int main(int, char **);

void libmain(int argc, char **argv) {
	// set env to point at our env structure in envs[].
	env = &envs[ENVX(syscall_getenvid())];

	// call user main routine
	printf("1\n");
	exit_status = main(argc, argv);
	printf("exit_status = %d\n", exit_status);
	syscall_exit(exit_status);

	// exit gracefully
	exit();
}
