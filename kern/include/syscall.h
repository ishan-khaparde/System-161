#ifndef _SYSCALL_H_
#define _SYSCALL_H_

/*
 *  * Prototypes for IN-KERNEL entry points for system call implementations.
 *   */
int sys_sbrk(int size, int *ret);
int sys_reboot(int code);
pid_t sys_getpid(int32_t *retval);
pid_t sys_fork(struct trapframe *tf, int32_t *retval);
int sys_execv(const char *progname, char **args, int *retval);
int sys_fsync(struct trapframe *tf);
int sys_write (struct trapframe *tf);
void sys__exit (int exitcode);
pid_t sys_waitpid(pid_t pid, int *status, int options, int *err);
#endif /* _SYSCALL_H_ */
