

#include <types.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <kern/limits.h>
#include <lib.h>
#include <addrspace.h>
#include <thread.h>
#include <curthread.h>
#include <vm.h>
#include <vfs.h>
#include <synch.h>

int sys_execv(const char *program, char **args, int *retval) {
    lock_acquire(execv_lock);
     vaddr_t entrypoint, stackptr;
    int result, i = 0;
    size_t arglen, size;
	
    /* Check if program name is specified or not 
 *        if the program field contains NULL then return 
 *               -1 & set error to EFAULT */

    if(program == '\0' || program == ' ')
    {
        *retval = -1;
        return EFAULT;
    }
    
    /* For execv we need to parse the arguments, from userland to
 *        kernel, we take temp variable to do all the copying
 *               temp_program contains the name of the executable
 *                      temp_argc is used to keep the argument count
 *                             temp_argv keeps the argument
 *                                 */
    
    char *temp_program = (char *)kmalloc(PATH_MAX);    
    
    /* copy the program name into temp_program */
    copyinstr((userptr_t)program,temp_program,PATH_MAX,&size);

    int temp_argc;	    

    /* count the number of arguments passed, temp_argc maintains this count
 *        we increase the count until we hit NULL */

    while(args[i] != NULL)
	{
            i++;
    	    temp_argc = i;
    	}	
    char **temp_argv;

    /* we need to take the argument and store it into the temp argument,
 *        after copying the arguments to temp we save it onto the kernel stack.
 *               But this is not the safe place, as the other process may write over the stack
 *                      & hence we need to copy it to the userland stack again. */

    temp_argv = (char **)kmalloc(sizeof(char*));   	
    for(i = 0; i < temp_argc; i++) 
    {
        int length_of_arg = strlen(args[i])+1;        
        temp_argv[i]=(char*)kmalloc(length_of_arg);
        /* copy the arguments passed to temp_argv */
        copyinstr((userptr_t)args[i], temp_argv[i], length_of_arg, &arglen);
    }

    /* adding NULL terminator to argument string */
    temp_argv[temp_argc] = NULL;

    /* open the file which is specified */
    struct vnode *temp;
    result = vfs_open(temp_program, O_RDONLY, &temp);
    if (result) 
    {
        *retval = -1;
        return result;
    }

    /* as soon as we have new executable, we replace the current address space
 *        & replace the old process image with a new process image 
 *               this is why we destroy the address space */

    if(curthread->t_vmspace != NULL)
    {
        as_destroy(curthread->t_vmspace);
        curthread->t_vmspace=NULL;
    }

    /* creating new address space */

    curthread->t_vmspace = as_create();
    if (curthread->t_vmspace==NULL) 
    {
        vfs_close(temp);
        *retval = -1;
        return ENOMEM;
    }

    as_activate(curthread->t_vmspace);

    /* load the elf, & after that close the vfs */

    result = load_elf(temp, &entrypoint);
    if (result) 
    {            
        vfs_close(temp);
        *retval = -1;
        return result;
    }

    vfs_close(temp);

    /* define the new stack for the new process */

    result = as_define_stack(curthread->t_vmspace, &stackptr);
    if (result)
    {        
        *retval = -1;
        return result;
    }

    /* now copy the argument from the kernel stack to the process stack
 *        which is on the user side */

    unsigned int temp_stack[temp_argc];	
    for(i = temp_argc-1; i >= 0; i--) 
    {
        /* while storing the arguments on to the stack, it is required
 *            that we should align each variable woth 4-byte boundries.
 *                       the code below adds the extra padding if the given variable 
 *                                  is not aligned */
        int length_of_args = strlen(temp_argv[i]);
        int padding = (length_of_args % 4);
        if(padding > 0)
	{
		do
		{
		length_of_args=length_of_args + 1;
		} while (length_of_args %4 !=0);
		padding = length_of_args;
	}
        else if(padding == 0)        
            padding = length_of_args;

        /* as the stack grows upwards, we substract the bytes needed for
 *            storing the variable, & then copy it from our temp_argv to the
 *                       newly made stack */
        stackptr = stackptr - (padding);
        copyoutstr(temp_argv[i], (userptr_t)stackptr, length_of_args, &arglen);
        temp_stack[i] = stackptr;
    }
        /* we again add a NULL terminator at the end, or we will receive
 *            the garbage values on pop */
    temp_stack[temp_argc] = (int)NULL;

    /* now we copy the new stack to the user process & enter the user mode */
    for(i = temp_argc-1; i >= 0; i--)
    {
        stackptr = stackptr - 4;
        copyout(&temp_stack[i] ,(userptr_t)stackptr, sizeof(temp_stack[i]));
    }

    *retval = 0;	
    kfree(temp_argv);
lock_release(execv_lock);	   
    md_usermode(temp_argc, (userptr_t)stackptr, stackptr, entrypoint);
    return EINVAL;
	}
