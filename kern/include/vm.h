#ifndef _VM_H_
#define _VM_H_

#include <machine/vm.h>
#include <bitmap.h>
#include "kern/types.h"

/* Fault-type arguments to vm_fault() */
#define VM_FAULT_READ        0    /* A read was attempted */
#define VM_FAULT_WRITE       1    /* A write was attempted */
#define VM_FAULT_READONLY    2    /* A write to a readonly page was attempted*/

#define VM_STACKPAGES    12

void vm_bootstrap(void);
int vm_up;

int vm_fault(int faulttype, vaddr_t faultaddress);

paddr_t getppages(unsigned long npages);


vaddr_t alloc_kpages(int npages);
void free_kpages(vaddr_t addr);




int add_ppage (u_int32_t vaddr, u_int32_t paddr,u_int32_t status, pid_t pid);
int remove_ppage (u_int32_t paddr);
void swapin(u_int32_t paddr, u_int32_t chunk);
void swapout( u_int32_t paddr,u_int32_t chunk);
u_int32_t get_ppage(u_int32_t vaddr, pid_t pid);
u_int32_t get_free_page();

struct contagious_frames{
vaddr_t vaddr;
size_t npages;
};

struct contagious_frames *used_frames;


#define IS_KERNEL(x) ((x) & 0x00000001)
#define IS_VALID(x) ((x) & 0x00000200)

void swap_cleanup();
/* possible status of page */
typedef enum
{
    FREE,
    DIRTY,
    CLEAN,
    KERNEL
} PAGE_STATUS;








#endif /* _VM_H_ */

