
#ifndef _SYNCH_H_
#define _SYNCH_H_



struct semaphore {
	char *name;
	volatile int count;
};

struct semaphore *sem_create(const char *name, int initial_count);
void              P(struct semaphore *);
void              V(struct semaphore *);
void              sem_destroy(struct semaphore *);



struct lock {
	char *name;
	volatile int lock_condition;
	volatile struct thread *lock_parent;
	
};

struct lock *lock_create(const char *name);
void         lock_acquire(struct lock *);
void         lock_release(struct lock *);
int          lock_do_i_hold(struct lock *);
void         lock_destroy(struct lock *);



struct cv {
	char *name;
	
};

struct cv *cv_create(const char *name);
void       cv_wait(struct cv *cv, struct lock *lock);
void       cv_signal(struct cv *cv, struct lock *lock);
void       cv_broadcast(struct cv *cv, struct lock *lock);
void       cv_destroy(struct cv *);

#endif /* _SYNCH_H_ */

