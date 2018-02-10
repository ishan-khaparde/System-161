#include <types.h>
#include <lib.h>
#include <synch.h>
#include <thread.h>
#include <curthread.h>
#include <machine/spl.h>


struct semaphore *
sem_create(const char *namearg, int initial_count)
{
	struct semaphore *sem;

	assert(initial_count >= 0);

	sem = kmalloc(sizeof(struct semaphore));
	if (sem == NULL) {
		return NULL;
	}

	sem->name = kstrdup(namearg);
	if (sem->name == NULL) {
		kfree(sem);
		return NULL;
	}

	sem->count = initial_count;
	return sem;
}

void
sem_destroy(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);

	spl = splhigh();
	assert(thread_hassleepers(sem)==0);
	splx(spl);

	
	kfree(sem->name);
	kfree(sem);
}

void 
P(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);

	
	assert(in_interrupt==0);

	spl = splhigh();
	while (sem->count==0) {
		thread_sleep(sem);
	}
	assert(sem->count>0);
	sem->count--;
	splx(spl);
}

void
V(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);
	spl = splhigh();
	sem->count++;
	assert(sem->count>0);
	thread_wakeup(sem);
	splx(spl);
}



struct lock *
lock_create(const char *name)
{
	struct lock *lock;

	lock = kmalloc(sizeof(struct lock));
	if (lock == NULL) {
		return NULL;
	}

	lock->name = kstrdup(name);
	if (lock->name == NULL) {
		kfree(lock);
		return NULL;
	}
	
	lock->lock_condition=0; 
	lock->lock_parent=NULL; 
	
	return lock;
}

void
lock_destroy(struct lock *lock)
{
	assert(lock != NULL);

	
	
	kfree(lock->name);
	kfree(lock);
}

void
lock_acquire(struct lock *lock)
{
	assert(lock!=NULL);
	
	int spl;
	spl=splhigh();
	while(lock->lock_condition==1){
		thread_sleep(lock);
	}
	lock->lock_condition=1;
	lock->lock_parent=curthread;
	splx(spl);
}

void
lock_release(struct lock *lock)
{
	int spl;
	spl=splhigh();
	lock->lock_condition=0;
	lock->lock_parent=NULL;
	thread_wakeup(lock);
	splx(spl);
}

int
lock_do_i_hold(struct lock *lock)
{
	int spl;
	spl=splhigh();
	if(lock->lock_parent==curthread)
	{
		splx(spl);
		return 1;
	}
	else
	{
		splx(spl);
		return 0;
	}
}



struct cv *
cv_create(const char *name)
{
	struct cv *cv;

	cv = kmalloc(sizeof(struct cv));
	if (cv == NULL) {
		return NULL;
	}

	cv->name = kstrdup(name);
	if (cv->name==NULL) {
		kfree(cv);
		return NULL;
	}
	
	
	
	return cv;
}

void
cv_destroy(struct cv *cv)
{
	assert(cv != NULL);

	
	kfree(cv->name);
	kfree(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	
	(void)cv;   
	(void)lock;  
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	
	(void)cv;    
	(void)lock;  
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	
	(void)cv;   
	(void)lock;  
}

