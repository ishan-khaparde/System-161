#include <synch.h>
#include <types.h>
#include <lib.h>
#include <kern/errno.h>
#include <array.h>
#include <machine/spl.h>
#include <machine/pcb.h>
#include <thread.h>
#include <curthread.h>
#include <vm.h>
#include <vnode.h>
#include <queue.h>


void
time_slice(void)
{
	int spl = splhigh();

	if(curthread == NULL) {
		return;
	}
	curthread->time_slice--;
	
	if(curthread->time_slice <= 0) {
		thread_yield();
	}
	splx(spl);
}

