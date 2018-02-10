#ifndef _COREMAP_H_
#define _COREMAP_H_


#include <machine/vm.h>
#include <bitmap.h>
#include "kern/types.h"

struct _PTE {
	paddr_t paddr; 
	vaddr_t vaddr; 
	u_int32_t recent_access_time;
	u_int32_t status; 
	pid_t pid;
};
void init_coremap();

#endif /* _COREMAP_H_ */
