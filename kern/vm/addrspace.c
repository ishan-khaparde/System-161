#include <types.h>
#include <vm.h>
#include <machine/tlb.h>
#include <curthread.h>
#include <kern/errno.h>
#include <thread.h>
#include <lib.h>
#include <addrspace.h>


#if OPT_DUMBVM

#else
/* allocate pages for kernel pages, other functions worked for user pages mostly */
u_int32_t as_page(u_int32_t vaddr, int pid)
{
    u_int32_t paddr;
    
    /* find the free page */
    paddr = get_free_page();

     if( vaddr <= MIPS_KSEG0)
    {
        paddr |= 0x00000200;
    }
    else
    {
        /* mark it as kernel page */
        paddr |= 0x00000001;
        paddr |= 0x00000200;
    }
    
    /*  update the entries in the add_ppage */
    add_ppage(vaddr, paddr, DIRTY, pid);
    
    return paddr;    
}


/* Create new address space, in the kernel, which is nothing but a page, which holds all the structure info */
struct addrspace *
as_create(void)
{
	
	struct addrspace *as = kmalloc(sizeof(struct addrspace));
	if (as==NULL) {
		return NULL;
	}
	/* this address space belong to current thread */
	as->pid = curthread->pid;

	/* initializing all the values to 0 */
	as->as_vbase1 = as->as_npages1 = as->as_pbase1 = 0;

	as->as_vbase2 = as->as_npages2 = as->as_pbase2 = 0;
	
	as->as_stackpbase = 0;

	as->as_heaptop = (vaddr_t)0;
	

	return as;
}

/* copy address space, we use memove to copy from one to another */
int as_copy(struct addrspace *old, struct addrspace **ret, pid_t pid)
{

	struct addrspace *new;

	new = as_create();
	if (new==NULL) {
		return ENOMEM;
	}

	new->pid = pid;

	/* inherit the values from the source addrspace */
	new->as_vbase1 = old->as_vbase1;
	new->as_npages1 = old->as_npages1;
	new->as_vbase2 = old->as_vbase2;
	new->as_npages2 = old->as_npages2;
	
	new->as_heaptop =  new->as_vbase2 + (new->as_npages2*PAGE_SIZE);
	
	/* load the new address space to memory, to proceed for copying */

	if (as_prepare_load(new)) {
		as_destroy(new);
		return ENOMEM;
	}

	/* start copying from old_as to new_as */
	int i;	
	u_int32_t old_vaddr, new_paddr;
	int temp = old->as_npages1;
	for (i=0; i < temp; i++)
	{
		old_vaddr = old->as_vbase1 + (i * PAGE_SIZE);

		new_paddr = get_ppage(new->as_vbase1 + (i * PAGE_SIZE), new->pid);
		new_paddr &= PAGE_FRAME;

		memmove((void *)PADDR_TO_KVADDR(new_paddr),(const void *) (old_vaddr),PAGE_SIZE);	
			
	}
	temp = old->as_npages2;
	for (i=0; i < temp; i++)
	{
	
		old_vaddr = old->as_vbase2 + (i * PAGE_SIZE);

		new_paddr = get_ppage(new->as_vbase2 + (i * PAGE_SIZE), new->pid);
		new_paddr &= PAGE_FRAME;
	
		memmove((void *) PADDR_TO_KVADDR(new_paddr),(const void *) (old_vaddr), PAGE_SIZE);	
	
	}

	for (i=0; i < VM_STACKPAGES; i++) 
	{
		old_vaddr = USERSTACK - ((i+1) * PAGE_SIZE);


		new_paddr = get_ppage(USERSTACK - ((i+1) * PAGE_SIZE), new->pid);
		new_paddr &= PAGE_FRAME;
		
		
		memmove((void *) PADDR_TO_KVADDR(new_paddr),(const void *) (old_vaddr), PAGE_SIZE);
		
	}
	for(;new->as_heaptop < old->as_heaptop;new->as_heaptop+=PAGE_SIZE) 
	{	
		old_vaddr = new->as_heaptop;

		new_paddr = get_ppage(new->as_heaptop, pid);
		new_paddr &= PAGE_FRAME;

		memmove((void *) PADDR_TO_KVADDR(new_paddr), (const void *)(old_vaddr), PAGE_SIZE);
    }
	
	/* return the copied as */
	*ret = new;
	/* indicate success */
	return 0;
}

/* destroy the addrspace */
void
as_destroy(struct addrspace *as)
{
	kfree(as);
}

/* In this function, we invalidate all the TLB */
void
as_activate(struct addrspace *as)
{
	TLB_Invalidate_all();

	(void)as;  
}

/* taken from DUMBVM */
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
		 int readable, int writeable, int executable)
{
	
	size_t npages; 

	/* Align the region. First, the base... */
	sz += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;

	/* ...and now the length. */
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = sz / PAGE_SIZE;

	/* We don't use these - all pages are read-write */
	(void)readable;
	(void)writeable;
	(void)executable;
	
		
	/* Note that we can arbitrarily set the number of pages we allocate
 *  * 	   and later swap code/data in and out, rather than the whole thing
 *   * 				at once. */

	if (as->as_vbase1 == 0) {
		as->as_vbase1 = vaddr;
		as->as_npages1 = npages;

		as->as_heaptop = vaddr + sz; 
		return 0;
	}

	if (as->as_vbase2 == 0) {
		as->as_vbase2 = vaddr;
		as->as_npages2 = npages;
		
		as->as_heaptop = vaddr + sz; 
		return 0;
	}

	/*
 *  * 	 * Support for more than two regions is not available.
 *   * 	 	 */
	kprintf("dumbvm: Warning: too many regions\n");
	return EUNIMP;
}

 /* This is used to load pages into the memory, for the use */
int
as_prepare_load(struct addrspace *as)
{
	int i;
	u_int32_t temp,temp1;
	int temp2;
	if(vm_up==0)
	{
		temp2 = as->as_npages1;
		for (i=0; i < temp2; i++)
	{
		
		temp1 = getppages(1);
		
		if((temp1 & PAGE_FRAME) == 0)
		{
			return ENOMEM;		
		}
	}

	}
		
	else
	{
	   temp2 = as->as_npages1;

		for (i=0; i < temp2; i++)
	{
		
			temp = as->as_vbase1 + (i*PAGE_SIZE);
			temp1 = as_page(temp, as->pid);
		
		if((temp1 & PAGE_FRAME) == 0)
		{
			return ENOMEM;		
		}
	}
		temp2 = as->as_npages2;

	for (i=0; i < temp2; i++)
	{
		
			temp = as->as_vbase2 + (i*PAGE_SIZE);
			temp1 = as_page(temp, as->pid);
		
		if((temp1 & PAGE_FRAME) == 0)
		{
			return ENOMEM;		
		}

	}
		
	for (i=0; i < VM_STACKPAGES; i++) 
	{
		
		
			temp = USERSTACK - ((i+1)*PAGE_SIZE);
			temp1 = as_page(temp, as->pid);
		

		if((temp1 & PAGE_FRAME) == 0)
		{
			return ENOMEM;		
		}
	}
	}
	
	
	return 0;
}


int
as_complete_load(struct addrspace *as)
{
	(void)as;
	return 0;
}


int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	/*
 *  * 	 * Write this.
 *   * 	 	 */
	(void)as;

	/* Initial user-level stack pointer */
	*stackptr = USERSTACK;
	
	return 0;
}

#endif


