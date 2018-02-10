/*
 *  *  * Sample/test code for running a user program.  You can use this for
 *   *   * reference when implementing the execv() system call. Remember though
 *    *    * that execv() needs to do more than this function does.
 *     *     */

#include <types.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <lib.h>
#include <synch.h>
#include <addrspace.h>
#include <thread.h>
#include <curthread.h>
#include <vm.h>
#include <vfs.h>

/*
 *  * Load program "progname" and start running it in usermode.
 *   * Does not return except on error.
 *    *
 *     * Calls vfs_open on progname and thus may destroy it.
 *      */

int
runprogram(char *progname, int argc, char **args)
{
	userptr_t start, userargv, arg, argv_base;
	vaddr_t entrypoint, stackptr;
	int result, i;
    size_t padding = 0;

    /* runprogram is similar to execv */
   	/* we allocate the space to store the argv passed from the userland */
    char **argv = kmalloc(sizeof(char*) * argc);
    /* to keep the track of the lengths, to adjust the stackpointer we use 
 *     length_of_arg */
    size_t *length_of_arg = kmalloc(sizeof(size_t) * argc);

    /* we now measure the the lengths of each arguments, & copy them into 
 *        new stack argv, which we will pass to further processes, as the current
 *               stack will modify with the new incoming processes */
    for (i = 0; i < argc; ++i) {
        size_t length_of_args = strlen(args[i]) + 1;
        argv[i] = kstrdup(args[i]);
        /*  we add the length to the padding, & we use it for keeping the 
 *              track of the pointer */
        length_of_arg[i] = padding;
        padding = padding + length_of_args;
    }

    struct vnode *temp;
	/* Open the file. */
	result = vfs_open(progname, O_RDONLY, &temp);
	if (result) {
		return result;
	}

	/* We should be a new thread. */
	assert(curthread->t_vmspace == NULL);

	/* Create a new address space. */
	curthread->t_vmspace = as_create();
	if (curthread->t_vmspace==NULL) {
		vfs_close(temp);
		return ENOMEM;
	}

	/* Activate it. */
	as_activate(curthread->t_vmspace);

	/* Load the executable. */
	result = load_elf(temp, &entrypoint);
	if (result) {
		/* thread_exit destroys curthread->t_vmspace */
		vfs_close(temp);
		return result;
	}

	/* Done with the file now. */
	vfs_close(temp);

	/* we make a new stack where we would keep the arguments */

	result = as_define_stack(curthread->t_vmspace, &stackptr);
	if (result) {
		return result;
	}

 	/* we copy the arguments to the usermode */

	vaddr_t stack = stackptr - padding;
	stack -= (stack & (sizeof(void *) - 1));
	start = (userptr_t)stack;
    
    for (i = 0; i < argc; ++i) {
        result = copyout(argv[i], start + length_of_arg[i], strlen(argv[i]) + 1);
        if (result)
            return result;
    }


	stack -= (argc + 1) * sizeof(userptr_t);
	userargv = (userptr_t)stack;
	argv_base = (userptr_t)stack;

	for (i = 0; i < argc; i++) {
		arg = start + length_of_arg[i];
		result = copyout(&arg, userargv, sizeof(userptr_t));
		if (result)
			return result;
		userargv += sizeof(userptr_t);
	}

	arg = NULL;
	result = copyout(&arg, userargv, sizeof(userptr_t));
	if (result)
		return result;

	stackptr = stack + sizeof(userptr_t);

	md_usermode(argc, argv_base,
		    stackptr, entrypoint);
	
	return EINVAL;
}

