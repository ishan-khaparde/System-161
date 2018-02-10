#ifndef _ADDRSPACE_H_
#define _ADDRSPACE_H_

#include <vm.h>
#include "opt-dumbvm.h"

struct vnode;

struct addrspace {
#if OPT_DUMBVM
	vaddr_t as_vbase1;
	paddr_t as_pbase1;
	size_t as_npages1;
	vaddr_t as_vbase2;
	paddr_t as_pbase2;
	size_t as_npages2;
	paddr_t as_stackpbase;
#else
	vaddr_t as_vbase1;
	paddr_t as_pbase1;
	size_t as_npages1;
	vaddr_t as_vbase2;
	paddr_t as_pbase2;
	size_t as_npages2;
	paddr_t as_stackpbase;
    vaddr_t as_heaptop;
    pid_t pid;
#endif
};


struct addrspace *as_create(void);
#if OPT_DUMBVM
int               as_copy(struct addrspace *src, struct addrspace **ret);
#else
int               as_copy(struct addrspace *src, struct addrspace **ret, pid_t pid);
#endif
void              as_activate(struct addrspace *);
void              as_destroy(struct addrspace *);
u_int32_t as_page(u_int32_t vaddr, int pid);
int               as_define_region(struct addrspace *as, 
				   vaddr_t vaddr, size_t sz,
				   int readable, 
				   int writeable,
				   int executable);
int		  as_prepare_load(struct addrspace *as);
int		  as_complete_load(struct addrspace *as);
int       as_define_stack(struct addrspace *as, vaddr_t *initstackptr);


int load_elf(struct vnode *v, vaddr_t *entrypoint);


#endif /* _ADDRSPACE_H_ */
