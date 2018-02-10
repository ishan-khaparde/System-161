#include <types.h>
#include <lib.h>
#include <thread.h>
#include <syscall.h>
#include <synch.h>
#include <kern/errno.h>
#include <curthread.h>
#include <addrspace.h>


pid_t
sys_waitpid(pid_t pid, int *status, int options, int *retval){

	if (options != 0){
		return EINVAL;	
	}
	
	if (status == NULL){
		return EFAULT;
	}
	
	/* to count the child we use variable child */
	int child = 0;

	struct pid_list *tmp = curthread->children;
	while (tmp != NULL){
		if (tmp->pid == pid){
			child = 1;
			break;
		}
		tmp = tmp->next;
	}

	/* if the given pid doesn't belong to the current thread, the child
 * 	   count is 0 & we exit */

	if (child<1){  
		status=0;
		thread_exit();
	}

	/* if the process is found & it is alive then we wait for the child */
	if(process_table[pid]->curr_state == ALIVE)
	{
		int temp = process_table[pid]->number_of_threads_waiting_on_you;
		process_table[pid]->number_of_threads_waiting_on_you=temp+1;
		P(process_table[pid]->waiting_on);

	}
	 /* if the process is dead then exit */
	else
	{
		status=0;
		thread_exit();
	}

	/* update the child list & exit */
	struct pid_list *curr = curthread->children;
	if (curr->pid == pid){
		curthread->children = curr->next;
		kfree(curr);
	}
	else{
		while (curr->next != NULL){
			if (curr->next->pid == pid){
				tmp = curr->next;
				curr->next = curr->next->next;
				kfree(tmp);
				break;
			}
			curr = curr->next;
		}
	}


	*status=0;
	*retval = pid;
	return 0;
}

