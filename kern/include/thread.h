#ifndef _THREAD_H_
#define _THREAD_H_

/*
 *  *  *  * Definition of a thread.
 *   *   *   */

/* Get machine-dependent stuff */
#include <machine/pcb.h>
/*  Required for pid_t typedef */
#include <types.h>

/*  Indicates the maximum number of scheduler priority levels we are
 *   *   *   * using. Also indicates how many queues we will use in our MLFQ.
 *    *    *    */
#define DEAD 0
#define ALIVE 1
/* Linked list of pids */
struct pid_list{
    pid_t pid;
    struct pid_list *next;
};

struct addrspace;

struct thread {
	/**********************************************************/
	/* Private thread members - internal to the thread system */
	/**********************************************************/
	
	struct pcb t_pcb;
	char *t_name;
	const void *t_sleepaddr;
	char *t_stack;
	
	/**********************************************************/
	/* Public thread members - can be used by other code      */
	/**********************************************************/
	
	/*
 *  *  * 	 * This is public because it isn't part of the thread system,
 *   *   * 	 	 * and will need to be manipulated by the userprog and/or vm
 *    *    * 	 	 	 * code.
 *     *     * 	 	 	 	 */
	
	int exit_status;
	int time_slice;
	struct pid_list *children;
	pid_t parent_pid;
	struct addrspace *t_vmspace;
	int number_of_threads_waiting_on_you;
	struct semaphore *waiting_on;
	/*
 *  *  * 	 * This is public because it isn't part of the thread system,
 *   *   * 	 	 * and is manipulated by the virtual filesystem (VFS) code.
 *    *    * 	 	 	 */
	struct vnode *t_cwd;

		struct addrspace *t_addrspace;	/* virtual address space */
	/*
 *  *  * 	 * 
 *   *   * 	 	 * Process Identifier field
 *    *    * 	 	 	 */
	pid_t pid;

	/*
 *  *  * 	 * 
 *   *   * 	 	 * Thread scheduler priority and clock tick
 *    *    * 	 	 	 */
	int curr_state;
	int is_user_thread;
	int priority;
	/*
 *  *  * 	 * Countdown timer to implement variable time quantum. For each call to
 *   *   * 	 	 * hardclock(), tick will be decremented. If/when it reaches zero, harclock
 *    *    * 	 	 	 * will force the thread to yield.
 *     *     * 	 	 	 	 */
	int tick;
};
extern struct lock *getpid_lock;
extern struct lock *execv_lock;
extern struct thread **process_table;
/*  check if a pid is already allocated */
int pid_taken(pid_t pid);

/*  allocate a unique pid to a process */
pid_t allocate_pid(void);

/* Call once during startup to allocate data structures. */
struct thread *thread_bootstrap(void);

/* Call during panic to stop other threads in their tracks */
void thread_panic(void);

/* Call during shutdown to clean up (must be called by initial thread) */
void thread_shutdown(void);

/*
 *  *  *  * Make a new thread, which will start executing at "func".  The
 *   *   *   * "data" arguments (one pointer, one integer) are passed to the
 *    *    *    * function.  The current thread is used as a prototype for creating
 *     *     *     * the new one. If "ret" is non-null, the thread structure for the new
 *      *      *      * thread is handed back. (Note that using said thread structure from
 *       *       *       * the parent thread should be done only with caution, because in
 *        *        *        * general the child thread might exit at any time.) Returns an error
 *         *         *         * code.
 *          *          *          */
int thread_fork(const char *name, 
		void *data1, unsigned long data2, 
		void (*func)(void *, unsigned long),
		struct thread **ret);
/*int thread_fork_userthread(const char *name,
 *                 void (*func)(void *, unsigned long),
 *                                 void *data1, unsigned long data2,
 *                                                 struct thread **ret);
 *                                                 */
int thread_fork_userthread(const char *name,
                void *data1, unsigned long data2,
                void (*func)(void *, unsigned long),
                struct thread **ret);

/*
 *  *  *  * Cause the current thread to exit.
 *   *   *   * Interrupts need not be disabled.
 *    *    *    */
void thread_exit(void);

/*
 *  *  *  * Cause the current thread to yield to the next runnable thread, but
 *   *   *   * itself stay runnable.
 *    *    *    * Interrupts need not be disabled.
 *     *     *     */
void thread_yield(void);

/*
 *  *  *  * Cause the current thread to yield to the next runnable thread, and
 *   *   *   * go to sleep until wakeup() is called on the same address. The
 *    *    *    * address is treated as a key and is not interpreted or dereferenced.
 *     *     *     * Interrupts must be disabled.
 *      *      *      */
void thread_sleep(const void *addr);

/*
 *  *  *  * Cause all threads sleeping on the specified address to wake up.
 *   *   *   * Interrupts must be disabled.
 *    *    *    */
void thread_wakeup(const void *addr);

/*
 *  *  *  * Return nonzero if there are any threads sleeping on the specified
 *   *   *   * address. Meant only for diagnostic purposes.
 *    *    *    */
int thread_hassleepers(const void *addr);


/*
 *  *  *  * Private thread functions.
 *   *   *   */

/* Machine independent entry point for new threads. */
void mi_threadstart(void *data1, unsigned long data2, 
		    void (*func)(void *, unsigned long));

/* Machine dependent context switch. */
void md_switch(struct pcb *old, struct pcb *nu);

void time_slice(void);

#endif /* _THREAD_H_ */
