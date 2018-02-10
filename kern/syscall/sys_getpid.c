#include <types.h>
#include <kern/unistd.h>
#include <thread.h>
#include <curthread.h>

int sys_getpid(int32_t* retval) {
	*retval = curthread->pid;
	return 0;
}
