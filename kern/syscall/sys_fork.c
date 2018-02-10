#include <types.h>
#include <lib.h>
#include <machine/spl.h>
#include <thread.h>
#include <addrspace.h> 
#include <curthread.h> 
#include <scheduler.h>
#include <vfs.h>
#include <kern/errno.h> 
#include <kern/limits.h> 
#include <machine/trapframe.h>
#include <synch.h> 
#include <syscall.h>
#include <vnode.h>
#include <uio.h>


struct init_data{
  struct semaphore *child_wait;
  struct semaphore *parent_wait;
  struct trapframe *child_tf;
};

static
void
md_forkentry(void *p){
  struct init_data *fork = (struct init_data *)p;
  P(fork->parent_wait);

  struct trapframe tf = *(fork->child_tf);
  tf.tf_v0 = 0;
  tf.tf_v1 = 0;
  tf.tf_a3 = 0;
  tf.tf_epc += 4; /* Advance program counter */
  as_activate(curthread->t_vmspace);

  V(fork->child_wait);

  mips_usermode(&tf);
}

pid_t sys_fork(struct trapframe *tf, int *err){

  struct init_data *fork = kmalloc(sizeof(struct init_data));
  if (fork == NULL){
    *err = ENOMEM;
    return -1;
  }
 
 /* create a new trapframe for the new child process */
  struct trapframe child_tf = *tf;
  fork->child_tf = &child_tf;

 /* we create the semaphore, for the synchronization purposes */

  fork->parent_wait = sem_create("wait on parent thread",0);
  if (fork->parent_wait == NULL){
    *err = ENOMEM;
     kfree(fork);
     return -1;
  }

  fork->child_wait = sem_create("wait on child thread",0);
  if (fork->child_wait == NULL){
    *err = ENOMEM;
    sem_destroy(fork->parent_wait);
    kfree(fork);
    return -1;
  }

  /* Create a pid_list entry for the parent */
  struct pid_list *new_child_pidlist = kmalloc(sizeof(struct pid_list));
  if (new_child_pidlist == NULL){
    *err = ENOMEM;

    sem_destroy(fork->parent_wait);
    sem_destroy(fork->child_wait);
    kfree(fork);
   return -1;
  }
  
   /* Create semaphore for the child process */
  struct semaphore *child_waiting_on = sem_create("waiting_on",0);
  if (child_waiting_on == NULL){
    *err = ENOMEM;

    kfree(new_child_pidlist);
    sem_destroy(fork->parent_wait);
    sem_destroy(fork->child_wait);
    kfree(fork);
   return -1;
  }
  
  
  struct thread *child_thread;
  /* here we actually fork the child using the thread_fork function */

  *err = thread_fork("child", fork, 0, md_forkentry, &child_thread);
  if (*err){
    kfree(new_child_pidlist);
    sem_destroy(child_waiting_on);
    sem_destroy(fork->parent_wait);
    sem_destroy(fork->child_wait);
    kfree(fork);
   return -1;
  }

  /* Copy the parent address space to the child*/
  /*struct addrspace *child_as;
  *err = as_copy(curthread->t_vmspace, &child_as,child_thread->pid);
  if (*err){
    *err = ENOMEM;
    kfree(new_child_pidlist);
    sem_destroy(child_waiting_on);
    sem_destroy(fork->parent_wait);
    sem_destroy(fork->child_wait);
    kfree(fork);
    return -1;
  }*/
 
 /* we assign the fields to the children processes */
 //child_thread->t_vmspace = child_as;
  
  VOP_INCREF(curthread->t_cwd); 
  child_thread->t_cwd =curthread->t_cwd;

  child_thread->waiting_on = child_waiting_on; 
  child_thread->parent_pid = curthread->pid;
  
  V(fork->parent_wait);
  P(fork->child_wait);

  int child_pid =child_thread->pid;
  new_child_pidlist->next = curthread->children;
  new_child_pidlist->pid = child_thread->pid;
  curthread->children = new_child_pidlist;  
  process_table[child_thread->pid] = child_thread;

  /* Free init_data (child is done) */
  sem_destroy(fork->parent_wait);
  sem_destroy(fork->child_wait);
  kfree(fork);


  return child_pid;

}
