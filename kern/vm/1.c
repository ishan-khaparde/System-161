#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <machine/spl.h>
#include <generic/random.h>
#include <clock.h>
#include <machine/trapframe.h>
#include <kern/unistd.h>
#include <machine/tlb.h>
#include <machine/bus.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/vm.h>
#include <bitmap.h>
#include <vfs.h>
#include <vnode.h>
#include <kern/stat.h>
#include <uio.h>
#include <thread.h>
#include <curthread.h>
#include <syscall.h>
#include <coremap.h>
#include <machine/tlb.h>


static int k;

/* Coremap structure to hold the values of vaddr & PID with respect to each page */
struct _PTE *coremap; 
struct _PTE *swaparea;

/* bitmap to manage swap & ram area */
struct bitmap *ram_memmap;
struct bitmap *swapdisk_memmap;

/* variable to store ram & swap area size */
int coremap_sz; 
int swaparea_sz;

/* variable to store ram & swaparea address*/
u_int32_t coremap_addr;
u_int32_t swap_addr;
 u_int32_t user_available_ram;

/* pointer for disk location */
struct vnode *swap_ptr;

/* possible status of page */
typedef enum
{    
    FREE,
    DIRTY,
    CLEAN,
    KERNEL
} PAGE_STATUS;

/* vm bootstrap function, initializes the VM at the time of system boot */
void vm_bootstrap(void)
{    
    int i=0,result;
    u_int32_t available_ram = mips_ramsize();
    /* Open the swap area file */
    char file_name[] = "lhd0raw:";     
    result = vfs_open(file_name, O_RDWR , &swap_ptr);
   
    struct stat swparea;    
    VOP_STAT(swap_ptr, &swparea);

    if(result)
    {
        panic("VM: Can't open file 1 lhd0raw ! Quiting..\n");
    }
    vm_up = 0;
    /* define swaparea & ram area used for swapping */
    
    /* this will keep the track of the frames that are being used & are occupied */
    used_frames = (struct contagious_frames*)kmalloc(64*sizeof(struct contagious_frames)); 

    /* calculate the swaparea_sz */
    swaparea_sz = swparea.st_size/PAGE_SIZE; 
    /* create the bitmap for each page in swaparea */  
    swapdisk_memmap = bitmap_create(swaparea_sz);    

    /* allocate swaparea in the memory */
    swaparea = (struct _PTE*)kmalloc(swaparea_sz * sizeof(struct _PTE));    
    swap_addr = 0; 

    /* save the addr of each swap page */
    i=0;
      do
        {
            swaparea[i].paddr = (i * PAGE_SIZE);
            i++;
        }while(i<swparea.st_size/PAGE_SIZE);
    
    /* initializing ram swap area*/
    /* get the ram which is available to us after initializing swaparea map*/
    user_available_ram = ram_stealmem(0);
    
    /* define the size for swap area in main memory*/
    coremap_sz = (available_ram-user_available_ram-PAGE_SIZE);
    coremap_sz = coremap_sz/PAGE_SIZE;

    /* create bitmap for page location in main memory */
    ram_memmap = bitmap_create(coremap_sz);
    coremap = (struct _PTE*)kmalloc(coremap_sz * sizeof(struct _PTE));
    
  /* initialize the addr, status &, pid of each swap page */
    user_available_ram = ram_stealmem(0);
   
        i=0;
        do
        {
        coremap[i].paddr = (user_available_ram + (i * PAGE_SIZE));
        coremap[i].status = FREE;
        i++;
        }while(i<coremap_sz);
     
    /* VM is now enabled & can be used */
   vm_up = 1;
}

/* add page */
int add_ppage (u_int32_t vaddr, u_int32_t paddr, u_int32_t status, pid_t pid)
{
    
    int spl,i;
    (void)status;
    /* calculate the page number from the paddr */
    paddr = paddr & PAGE_FRAME;
    
    spl=splhigh();  
    /* calculate the page index & save the vaddr */
    i = (paddr - user_available_ram);
    i= i / PAGE_SIZE;
    
    coremap[i].vaddr = vaddr;
    /* if the virtual addr is not in kernel space then 
 *        set it as dirty, valid & as a kernel page. */

    /* update the entry in the coremap as dirty */    
    coremap[i].status = DIRTY;

    if(vaddr < USERTOP)
    {
       coremap[i].paddr |= 0x00000200;
       coremap[i].paddr |= 0x00000400;

    }
        
    /* if the virtual addr is not in the user space then 
 *        set it as dirty, valid & kernel page */
    else
    {

        coremap[i].paddr |= 0x00000200;
        coremap[i].paddr |= 0x00000400;
        coremap[i].paddr |= 0x00000001;
    }
    
    /* also write other info about the page */
    coremap[i].pid = pid;

    /* also write timestamp for LRU */
    coremap[i].recent_access_time=0;

    /* mark the bitmap */
    if(!bitmap_isset(ram_memmap,i))
    {
        bitmap_mark(ram_memmap, i); 
    }
           
    splx(spl);
        
    return 0;

}

/* remove page */
int remove_ppage (u_int32_t paddr)
{   
    
    int spl,i;


    /* calculate the page number from the paddr */
    paddr = paddr & PAGE_FRAME;

    spl=splhigh();
    /* calculate the page index & save the vaddr */    
    i = (paddr - user_available_ram);
    i=i / PAGE_SIZE;
    /* mark the page as free */
    coremap[i].status = FREE;

    /* update the entries in the coremap */
    coremap[i].vaddr = 0;
    u_int32_t temp = paddr & PAGE_FRAME;
    coremap[i].paddr = temp;

    /* also store default value in all reg */
    coremap[i].recent_access_time = 0;
    coremap[i].pid = 0;
    
    /* unmark the bitmap */
    if(bitmap_isset(ram_memmap,i))
    {

        bitmap_unmark(ram_memmap, i); 
    }

    splx(spl);  
    
    return 0;
}

/* swap the page onto file */
void swapin(u_int32_t paddr, u_int32_t block_addr)
{
    
    int spl;
   
    struct uio swap_block;

    /* calculate the page number from the paddr */
    u_int32_t temp= paddr & PAGE_FRAME;
    /* covert to vaddr */
    temp = PADDR_TO_KVADDR (temp);

    spl=splhigh(); 

    /*initializing the read I/O */
    mk_kuio(&swap_block,(void*)temp,PAGE_SIZE,block_addr, UIO_READ);        
    
    int j = (block_addr & PAGE_FRAME);
    j=j / PAGE_SIZE;

    swaparea[j].paddr = block_addr;

    /* mark the swapped area in the coremap as free by setting default values */
    swaparea[j].vaddr = 0;
    swaparea[j].pid = 0;    
    
    if(bitmap_isset(ram_memmap,j))
    {
        bitmap_unmark(swapdisk_memmap, j); 
    }

    splx(spl);
    
    /* swap the page*/
    int result=VOP_READ(swap_ptr, &swap_block);
    if(result) 
    {
        panic("VM: Can't swap the page onto memory! Quiting..");
    }
            
}

void swapout(u_int32_t chunk, u_int32_t paddr)
{    
    int spl,pid;
    u_int32_t vaddr;  
    struct uio swap_block;
    /* calculate the page number from the paddr */
    u_int32_t temp= paddr & PAGE_FRAME;
    /* covnert to vaddr */
    temp = PADDR_TO_KVADDR (temp);

    spl=splhigh();  
    /* initialize the UIO */
    mk_kuio(&swap_block,(void*)temp,PAGE_SIZE,chunk, UIO_WRITE);

    struct _PTE ppage = coremap[(paddr-user_available_ram)/PAGE_SIZE];
    
    /* update the swaparea coremap */
    pid = ppage.pid;
    vaddr=ppage.vaddr;
        
    int k = (chunk & PAGE_FRAME) / PAGE_SIZE;

    swaparea[k].vaddr = vaddr;
    swaparea[k].recent_access_time = 0;
    swaparea[k].pid = pid; 

    /*set bitmap as allocated */
    if(!bitmap_isset(swapdisk_memmap, k))
    {
        bitmap_mark(swapdisk_memmap, k);
    }
        
    
    /* swap out the page */
    int result=VOP_WRITE(swap_ptr, &swap_block);
    if(result)     
    {
        panic("VM: Failed to swap page from disk! Quiting..");           
    }
    
    /* if written successfully, then invalidate the specific TLB */

    TLB_Invalidate(paddr);
    splx(spl);
}



/* get free page */
u_int32_t get_free_page() 
{
    /* same as free block, but we find page in memory here */
    int spl=splhigh();
    unsigned j; 
    int i;
    u_int32_t old_page;
    time_t sec; 
    u_int32_t nsec;
    /* find the empty bitmap in ram memory map */
    int temp = bitmap_alloc(ram_memmap, &j);
        
    /* if found, return the paddr*/    
    if(!temp)
    {
        paddr_t paddr = coremap[j].paddr;
        return paddr;
    }   
    /* if the free page is not found, then we must replace page,
 *        to get the free page, & we are using LRU algorithm */     
    else
    {

    /* get the current time */
    gettime(&sec, &nsec);        
    u_int32_t currsec=sec;

    /* search for the page with the lowest time */
    for(i=0;i< (int)coremap_sz;i++)
    {
        if (coremap[i].recent_access_time <= currsec)
        {   /* make sure we are not swapping the kernel page */
            if(!IS_KERNEL(coremap[i].paddr))
                {
                    currsec = coremap[i].recent_access_time;
                    old_page = i;
                }
        }
    }

    }

    /* get the paddr of the old page */
    u_int32_t paddr=(coremap[old_page].paddr);
    /* make sure that paddr is valid */
    if(IS_VALID(paddr)) 
        {
            /* get a free block in the swaparea & swap the old page to file swaparea */
            splx(spl);
            u_int32_t chunk;
            /* find the empty bitmap  which is not allocated */
            unsigned t;
            int temp = bitmap_alloc(swapdisk_memmap, &t);
            /* if not found,then exit */
            if(temp)
            {
            splx(spl);
            kprintf("VM: Swap Space full, killing current thread! ");
            thread_exit();
            return -1;
            }  

            /* return the block*/
            else
            {
            chunk =(t*PAGE_SIZE);  
            }
            swapout(chunk, paddr);            
            
        }       
        return paddr;     
}


/* get phystcal page back from swaparea*/
u_int32_t get_ppage(u_int32_t vaddr, pid_t pid)
{
    u_int32_t paddr;    
    int i;
    u_int32_t block_addr;

    /* find the virtual address in the main memory first, before looking in swaparea*/
    int spl=splhigh();
    for(i=0; i < (int)coremap_sz; i++) 
    {
        if(coremap[i].vaddr == vaddr) 
        {          
                /* make sure it is valid */
            if(IS_VALID(coremap[i].paddr)) 
                {             
                splx(spl);
                /* return the page address */
                return coremap[i].paddr;
                }
            
            
        }        
       
       
    }
 
 /* search the swaparea & if the correosponding entry is found then copy the addr */
    for(i=0; i < (int)swaparea_sz; i++) 
        {
        
        if(swaparea[i].vaddr == vaddr)
        {            
            block_addr=(swaparea[i].paddr);
        }
    }

    /* find the free page in the main memory for swapping */
    paddr = get_free_page();
    /* swap the page */    
    swapin(paddr, block_addr);
    /* mark the page as valid & swapped */
 
    paddr |= 0x00000200;    
    paddr |= 0x00000080;
    
    /* update entries in add_ppage */
    add_ppage(vaddr, paddr, CLEAN, pid); 
    splx(spl);
    /* return the physical addr */
    return paddr;   
  
}


/* allocate pages for kernel pages, other functions worked for user pages mostly */
u_int32_t alloc_page(u_int32_t vaddr, int pid)
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

/* from DUMBVM */
paddr_t
getppages(unsigned long npages)
{
    int spl;
    paddr_t paddr;

    spl = splhigh();

    paddr = ram_stealmem(npages);
    
    splx(spl);
    return paddr;
}


/* free the pages */
void 
free_kpages(vaddr_t vaddr)
{
    int i=0;
    int index = 0;
    int found; 
    do
    {
        if(used_frames[i].vaddr==vaddr)
        {
            index=i;
            found =1;
            break;
        }
        i++;
    } while (i<k);
        
    
 if(found==1)
 {
    /* remove the pages, & update the entries */
    int count = (int) used_frames[index].npages;
    
     /* we have to remove 'count' pages */
    i=0;
    u_int32_t paddr;
    
    do
      {
        paddr = (vaddr+(i*PAGE_SIZE))-MIPS_KSEG0;
        remove_ppage(paddr);     
        i++;
      }while(i<count);   
    
    /* update frame maps to indicated frames which are free */
    used_frames[index].npages=0;
    used_frames[index].vaddr=0;
 }
    

    
}

/* alloc kernel pages */
vaddr_t 
alloc_kpages(int npages)
{
    
    paddr_t paddr;
    vaddr_t vaddr;
            
    
    /* if vm_up is not up then use the DUMBVM getppages*/
    if(!vm_up)
    {
        paddr = getppages(npages);
        vaddr = PADDR_TO_KVADDR(paddr);
        return vaddr;
    }
    else
    {
    int spl=splhigh();
    int i,n,pid;
    n=npages;
    pid= curthread->pid;
    unsigned index=0,count=0;
    /* if we need to allocated only 1 page */
    if(n==1) 
    {   /* find a single page */
        paddr = get_free_page();
         /* mark it as valid & kernel page */
        paddr |=0x00000200; 
        paddr |=0x00000001;
        /* convert to vaddr */
        int x =  PADDR_TO_KVADDR(paddr);
        /* update the entries in the add_ppage */
        add_ppage(x, paddr, DIRTY, curthread->pid);
        splx(spl);
        paddr=(paddr & PAGE_FRAME);
        used_frames[k].vaddr=vaddr;
        used_frames[k].npages=npages;
        k++; 
        goto x;
    }

    /* if we need to allocate more pages, we need to find position, where all pages can fit together */
    for(i = 0 ; i < (int)coremap_sz; i++) 
    {
        if(IS_KERNEL(coremap[i].paddr)) 
        {
            count = 0;
        }
        else 
        {
            if( count == 0)
            {
                index = i;
            }
            if( !bitmap_isset(ram_memmap, i) )        
            { 
                count++;
            }
            else
            {
                count=0;
            }

            if ((int)count >= n)
            {
                goto z;
            }
        }
    }

    for(i = 0 ; i < (int)coremap_sz; i++) 
    {
        if(IS_KERNEL(coremap[i].paddr)) 
        {
            count = 0;
        }
        else 
        {
            if( count == 0)
            {
                index = i;
            }
                count++;

            if ((int)count >= n)
            {
                break;
            }
        }
    }

         
            /* if yes then we can find the free pages from here */
            z:index = i;
       
        /* if we have enough pages then break */
        if((int)count < n) 
        {
        return ENOMEM;
        }                  
    
    /* now add the new pages entries */
    for(i=index ; i < (int)(index + n); i++) 
    {
        u_int32_t temp= PADDR_TO_KVADDR(coremap[i].paddr);
        add_ppage(temp, coremap[i].paddr, DIRTY, pid);
    }
    splx(spl);    
    
    paddr = coremap[index].paddr;
    paddr=(paddr & PAGE_FRAME);
    }
        
    /* return the addresses */
    x:if (paddr!=0) 
    {
        vaddr = PADDR_TO_KVADDR(paddr);
        if(npages>1)
      {       
        used_frames[k].vaddr=vaddr;
        used_frames[k].npages=npages;
        k++; 
    }
        
    }
    else
    {
        return 0;
    }
    /* if more than one page was sent then mark the frames being used */
    
    
    return vaddr;
}


int
vm_fault(int faulttype, vaddr_t faultaddress)
{
    

    paddr_t paddr=0;
    struct addrspace *as;
    int spl;


    faultaddress &= PAGE_FRAME;

        if(faultaddress != 0)
        {
            switch (faulttype) 
            {
                case VM_FAULT_READONLY:
                panic("vm: got VM_FAULT_READONLY\n");
                case VM_FAULT_READ:
                case VM_FAULT_WRITE:

                as = curthread->t_vmspace;

                if (as == NULL) 
                {
                    DEBUG(DB_VM, "No address space set up!\n");
                    return EFAULT;
                }

                /* make sure its a user space */
                u_int32_t vaddr= faultaddress;
                /* get a page */
                paddr = get_ppage(vaddr & PAGE_FRAME, curthread->pid);
                /* if it is a valid paddr then update the coremap entry */
                if( IS_VALID(paddr) ) 
                {
                    time_t sec; 
                    u_int32_t nsec;
                    gettime(&sec, &nsec);
        
                    int z = ((paddr & PAGE_FRAME)-user_available_ram) / PAGE_SIZE;
                    coremap[z].recent_access_time = (u_int32_t)sec;
                    paddr=(paddr & PAGE_FRAME);
                
                  
                /* return the addresses & insert TLB */
                if (paddr == 0)
                {
                panic("VM FAULT: Error !! Quiting..\n");
                }


//                paddr = paddr & PAGE_FRAME;

                spl = splhigh(); 
                TLB_Insert(faultaddress, paddr);
                splx(spl);      
                return 0;
    }
                break;
                default:
                return EFAULT;
            }

        return EFAULT;
        }
        else
        {
            return 0;
        }

    
}

/*
void lru_dec()
{
int i;
for(i=0;i< (int)coremap_sz;i++)
    {

        if(coremap[i].recent_access_time !=0)
        {
        coremap[i].recent_access_time -=1;
        }
   }
}
*/

void swap_cleanup()
{
        int i;
    int pid = curthread->pid;
    int count = swaparea_sz;
    for(i=0; i <count; i++)
    {
        if(swaparea[i].pid == pid)
        {
            if(bitmap_isset(swapdisk_memmap,i))
                {
                bitmap_unmark(swapdisk_memmap, i);
                } 
        swaparea[i].vaddr = 0;
        swaparea[i].recent_access_time = 0;
        swaparea[i].pid = 0;
        }
     
    }
 for(i=0; i <coremap_sz; i++)
    {
        if(coremap[i].pid == pid)
        {
            if(bitmap_isset(ram_memmap,i))
                {
                bitmap_unmark(ram_memmap, i);
                } 
        coremap[i].vaddr = 0;
        coremap[i].status = FREE;
        coremap[i].recent_access_time=0;
        coremap[i].pid = 0;
        }

}
}

