#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <machine/pcb.h>
#include <machine/spl.h>
#include <machine/trapframe.h>
#include <kern/callno.h>
#include <kern/unistd.h>
#include <syscall.h>
#include <thread.h>
#include <synch.h>
#include <uio.h>
#include <vfs.h>
#include <vnode.h>
#include <curthread.h>
#include <machine/vm.h>
#include <vm.h>
#include <kern/stat.h>
#include <fs.h>
#include "addrspace.h"


int sys_sbrk(int size, int *retval)
{
    /* initialization */
    struct addrspace *aux_address_space;
    vaddr_t heap_first_element;
    size_t number_of_pages;
    u_int32_t index;
    paddr_t result_alloc;
    
    /* copying the current addrspace to an auxillary as */
    aux_address_space = curthread->t_vmspace;        
    heap_first_element=aux_address_space->as_heaptop;

    /* Error Checking */
    /* if the size of addrspace is greater than the available heap, then indicated fault */

    if( (aux_address_space->as_heaptop+(number_of_pages* PAGE_SIZE) ) > (USERSTACK + (VM_STACKPAGES * PAGE_SIZE)) ) 
    {
        *retval = -1;
        return EINVAL; 
    }

    /* the value of size can't be zero or negative in sbrk function */
    if(size == 0)
    {
        *retval = heap_first_element;
        return 0;
    }
    else if(size < 0) 
    {
        *retval = -1;
        return EINVAL;
    }
    
    /* divide the size into page size of 4096 & then call alloc_page function to allocated a page */
    number_of_pages = (size / PAGE_SIZE)+1;

    
    for(index=0;index<number_of_pages;index++)
    {
        u_int32_t vaddr = aux_address_space->as_heaptop+(index*PAGE_SIZE);
        int pid = aux_address_space->pid;
        u_int32_t paddr;
        if(vm_up!= 0)
        {   
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
        
            /* in case of dumbvm is set */
            if(paddr == 0) 
            {
            *retval = -1;
            return ENOMEM;      
            }

        }
        else
        {
                result_alloc=getppages(1); 
                if(result_alloc == 0) 
                {
                    *retval = -1;
                    return ENOMEM;      
                }         
        }
        
        
    }
    
    /* update the heaptop */
    aux_address_space->as_heaptop = aux_address_space -> as_heaptop;
    aux_address_space->as_heaptop += (number_of_pages * PAGE_SIZE);
    
    *retval = heap_first_element;

    return 0;
}


