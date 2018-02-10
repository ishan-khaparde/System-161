#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <curthread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/spl.h>
#include <machine/tlb.h>
#include <clock.h>

/* function to insert a TLB */
int TLB_Insert(vaddr_t faultaddr, paddr_t paddr)
{
	
	int i=0;
	u_int32_t entryhi, entrylo,free_entry, nsec;
    time_t sec; 
	/* Read each TLB, & if found free then, write it */
	do{
		TLB_Read(&entryhi, &entrylo, i);
		if (!(entrylo & TLBLO_VALID)) 
		{
		entrylo = paddr | TLBLO_DIRTY | TLBLO_VALID;
		TLB_Write(faultaddr, entrylo, i);
		gettime(&sec, &nsec);        
		recent_access_time[i] = nsec;	
		return 0;
		}
	i++;	
	}while(i<NUM_TLB);
	/* if no entries are found, then we must replace the TLB's*/
	/* We are replaing the TLB using LRU, least recently used
 * 	   A TLB has timestap which is attached to it, which is compared
 * 	   	   & the TLB with the oldest time is swapped */

gettime(&sec, &nsec);        
    u_int32_t temp1= nsec;

	for (i=0; i<NUM_TLB; i++) {
		if(TLB_Probe(faultaddr, 0))
			{
				recent_access_time[i] = nsec;
			}
		}
		free_entry = 0;
		i=0;
				do{
					if(  recent_access_time[i] <= temp1)
					{
						temp1 = recent_access_time[i];
						free_entry = i;
					}
					i++;
				}while(i< NUM_TLB);
		 		
			
		entrylo = paddr | TLBLO_DIRTY | TLBLO_VALID;
		TLB_Write(faultaddr, entrylo, free_entry);
		gettime(&sec, &nsec);        
		recent_access_time[i] = nsec;	                          	
		return 0;
}

/* from DUMBVM */
int TLB_Invalidate(paddr_t paddr)
{
    u_int32_t ehi,elo;

    int i =0,spl;
    spl=splhigh();
    do{
    	TLB_Read(&ehi, &elo, i);
    	u_int32_t temp = elo & 0xfffff000;
    	u_int32_t temp1 = paddr & 0xfffff000;
        if (temp == temp1)	
        {
            TLB_Write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
            recent_access_time[i]=0;
            		
        }
		i++;
	  }while(i< NUM_TLB);
	  splx(spl);
    return 0;
}

/* same as above but invalidates TLB */
int TLB_Invalidate_all()
{
	int i,spl;
	
	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		TLB_Write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}
		for (i=0; i<NUM_TLB; i++)
		{
			recent_access_time[i] = 0;
		}
			
	splx(spl);

	return 0;
}




