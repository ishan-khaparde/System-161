

/* Include   */

#include <types.h>
#include <lib.h>
#include <test.h>
#include <thread.h>
#include <synch.h>
#include <machine/spl.h>
#include <curthread.h>


/*
 *  *  *
 *   *   * Variables
 *    *    *
 *     *     */
int count=0; /* counts number of cars */
int st_pass=0; /* stores number of cars going straight */
int lft_pass=0; /* stores number of cars going left */


/*
 *  *  * Number of cars created.
 *   *   */

#define NCARS 20

/*
 *  *  * Defining the directions
 *   *   */

#define NORTH 0
#define EAST  1
#define SOUTH 2
#define WEST  3

/*
 *  *  * Defining the turning directions
 *   *   */
#define STRAIGHT 0
#define RIGHT 1
#define LEFT  2


/*
 *  *  * Defining the Locks
 *   *   */
struct lock *SW;
struct lock *NW;
struct lock *NE;
struct lock *SE;
struct lock *PRINT;
struct lock *LL;
struct lock *SL;

/*
 *  *  * Defining the Lock values
 *   *   */
#define NORTHEAST 1
#define SOUTHEAST 5
#define SOUTHWEST 13
#define NORTHWEST 9

/*
 *  *  *
 *   *   * Function Definitions
 *    *    *
 *     *     */


void print_side(int cardirection) /* print sides using cardirection value */
{

       switch(cardirection)
        {
            case NORTH:
            kprintf("North");
            break;
        
            case EAST:
            kprintf("East");
            break;
        
            case SOUTH:
            kprintf("South");
            break;
        
            case WEST:
            kprintf("west");
            break;
    
        }
        
}


void print_info1(unsigned long int carnumber, int cardirection) /* print cardirection & carnumber using values */
{
        kprintf("Car No:%lu, from ", carnumber);
        print_side(cardirection);      
}    
       
void get_lock (int choose_locks) 
{
       switch(choose_locks) /* acquire lock 1,2 & 3 */
    {
        case NORTHEAST:
        lock_acquire(NE);
        break;
        
        case SOUTHEAST:
        lock_acquire(SE);
        break;
        
        case SOUTHWEST:
        lock_acquire(SW);
        break;
        
        case NORTHWEST:
        lock_acquire(NW);
        break;

        case 0:
        break;
    }
}

void getlock (int choose_locks1, int choose_locks2, int choose_locks3)
{  /* get the lock values from turning function & then call get_lock function to acquire locks */
    get_lock(choose_locks1);
    get_lock(choose_locks2);
    get_lock(choose_locks3);

}

void release_lock(int choose_locks)
{
    switch(choose_locks) /* release those locks which are acquired */
    {
        case NORTHEAST:
        lock_release(NE);
        break;
        
        case SOUTHEAST:
        lock_release(SE);
        break;
        
        case SOUTHWEST:
        lock_release(SW);
        break;
        
        case NORTHWEST:
        lock_release(NW);
        break;

        case 0:
        break;
    }
}

void releaselock (int choose_locks1, int choose_locks2, int choose_locks3)
{  /* get the acquired lovk and pass it to release_lock to release locks */
    release_lock(choose_locks1);
    release_lock(choose_locks2);
    release_lock(choose_locks3);
}

/*
 *  *  * gostraight()
 *   *   *
 *    *    * Arguments:
 *     *     *      unsigned long cardirection: the direction from which the car
 *      *      *              approaches the intersection.
 *       *       *      unsigned long carnumber: the car id number for printing purposes.
 *        *        *
 *         *         * Returns:
 *          *          *      nothing.
 *           *           *
 *            *            * Notes:
 *             *             *      This function should implement passing straight through the
 *              *              *      intersection from any direction.
 *               *               *      Write and comment this function.
 *                *                */  

static
void
gostraight(int cardirection,
           unsigned long int carnumber,
          int choose_locks1,
          int choose_locks2,
          int choose_locks3) 
{
        lock_acquire(PRINT);    /* acquire lock to print */
        kprintf("Car No:%lu, approaching at ", carnumber); /* print the info*/
        print_side(cardirection);
        kprintf(" , wants to go Straight.\n");
        lock_release(PRINT);  /* release lock */
       
        /* the car can't acquire the lock if there is a car going left or 2 cars going straight simultaneously*/
        while(lft_pass>0 && st_pass>2); 
        st_pass++; /* increase the value of st_pass straight going cars */
        lock_acquire(SL); /* first car to arrive will choose locks for going straight */
        getlock(choose_locks1,choose_locks2,choose_locks3);
        lock_release(SL); /* release lock so that other car can choose lock */
        st_pass--; /* after passing remove the lock value */

        lock_acquire(PRINT); /* acquire the lock & let the car print message */
        print_info1(carnumber, cardirection);
        kprintf(" crossing the Podunk junction for going Straight\n");
        lock_release(PRINT); /* release lock so that other car can also print message */
       
        releaselock(choose_locks1,choose_locks2,choose_locks3);
                /* released locks after going straight */
        
}


/*
 *  *  * turnleft()
 *   *   *
 *    *    * Arguments:
 *     *     *      unsigned long cardirection: the direction from which the car
 *      *      *              approaches the intersection.
 *       *       *      unsigned long carnumber: the car id number for printing purposes.
 *        *        *
 *         *         * Returns:
 *          *          *      nothing.
 *           *           *
 *            *            * Notes:
 *             *             *      This function should implement making a left turn through the 
 *              *              *      intersection from any direction.
 *               *               *      Write and comment this function.
 *                *                */

static
void
turnleft(int cardirection,
         unsigned long int carnumber,
          int choose_locks1,
          int choose_locks2,
          int choose_locks3)
{
        lock_acquire(PRINT);   /* acquire lock to print */ 
        kprintf("Car No:%lu, approaching at ", carnumber);
        print_side(cardirection);
        kprintf(" , wants to go Left.\n");
        lock_release(PRINT);/* release lock */
        
        /* if there is any car going straight or left then the car has to wait */
        while(st_pass>0 && lft_pass>0 );
        lft_pass++; /* increase the counter for cars going left*/
        lock_acquire(SL); /* acquire lock of going straight, don't let any car go straight when a car is going left */
        getlock(choose_locks1,choose_locks2,choose_locks3);
        lock_release(SL);
        lock_release(LL); /* release both left & straight locks so others can acquire */
        lft_pass--; /* decrement the counter of cars going left */

        lock_acquire(PRINT); /* acquire lock to print the information */
        print_info1(carnumber, cardirection);
        kprintf(" crossing the Podunk junction for going Left\n");
        lock_release(PRINT); /* leave the lock so others can print */

        releaselock(choose_locks1,choose_locks2,choose_locks3);
        /* released locks after going left */
               
}


/*
 *  *  * turnright()
 *   *   *
 *    *    * Arguments:
 *     *     *      unsigned long cardirection: the direction from which the car
 *      *      *              approaches the intersection.
 *       *       *      unsigned long carnumber: the car id number for printing purposes.
 *        *        *
 *         *         * Returns:
 *          *          *      nothing.
 *           *           *
 *            *            * Notes:
 *             *             *      This function should implement making a right turn through the 
 *              *              *      intersection from any direction.
 *               *               *      Write and comment this function.
 *                *                */

static
void
turnright(int cardirection,
          unsigned long int carnumber,
          int choose_locks1,
          int choose_locks2,
          int choose_locks3)
{       
        lock_acquire(PRINT);    /* acquire lock to print */
        kprintf("Car No:%lu, approaching at ", carnumber);
        print_side(cardirection);
        kprintf(" , wants to go Right.\n");
        lock_release(PRINT);/* release lock */
       
        getlock(choose_locks1,choose_locks2,choose_locks3);
        
        lock_acquire(PRINT); /* acquire lock to print the information */
        print_info1(carnumber, cardirection);
        kprintf(" crossing the Podunk junction for going Right\n");
        lock_release(PRINT); /* leave the lock so others can print */
        
        releaselock(choose_locks1,choose_locks2,choose_locks3);  
                   /* released locks after going right */   
}


/*
 *  *  * approachintersection()
 *   *   *
 *    *    * Arguments: 
 *     *     *      void * unusedpointer: currently unused.
 *      *      *      unsigned long carnumber: holds car id number.
 *       *       *
 *        *        * Returns:
 *         *         *      nothing.
 *          *          *
 *           *           * Notes:
 *            *            *      Change this function as necessary to implement your solution. These
 *             *             *      threads are created by createcars().  Each one must choose a direction
 *              *              *      randomly, approach the intersection, choose a turn randomly, and then
 *               *               *      complete that turn.  The code to choose a direction randomly is
 *                *                *      provided, the rest is left to you to implement.  Making a turn
 *                 *                 *      or going straight should be done by calling one of the functions
 *                  *                  *      above.
 *                   *                   */
 
static
void
approachintersection(void * unusedpointer,
                     unsigned long int carnumber)
{
        int cardirection;
        int departing_direction;
        (void) unusedpointer;
      
        cardirection = random() % 4;
        departing_direction=random() % 3; 
        
        /* variable for choosing locks, this variable holds the values of the lock which has to be acquired*/
        /* if the value comes out to be 13, it means it needs SOUTHWEST lock */
        int choose_locks1=0;
        int choose_locks2=0;
        int choose_locks3=0;
        int temp = cardirection;

        count ++; /* count the number of cars */

        /*calculate the value of first lock for going right*/
        choose_locks1=(((temp+4) % 4)*((temp+4) % 4)) + (((temp+3) % 4)*((temp+3) % 4));
        if(departing_direction==RIGHT)
        {
            turnright(cardirection,carnumber,choose_locks1,choose_locks2,choose_locks3);
        }

        /*calculate the value of second lock while going straight */
        choose_locks2=(((temp+3) % 4)*((temp+3) % 4)) + (((temp+2) % 4)*((temp+2) % 4));
        if(departing_direction==STRAIGHT)
        {
            
            gostraight(cardirection,carnumber,choose_locks1,choose_locks2,choose_locks3);
        }

        /*calculate the value of third lock for going left */
        choose_locks3=(((temp+2) % 4)*((temp+2) % 4)) + (((temp+1) % 4)*((temp+1) % 4));
        if(departing_direction==LEFT)
        {
            lock_acquire(LL); /* acquire lock for going left, as only one car can pass left at one time*/
            turnleft(cardirection,carnumber,choose_locks1,choose_locks2,choose_locks3);
        }

        lock_acquire(PRINT); /* again take lock to print meassage while leaving */
        print_info1(carnumber, cardirection);
        kprintf(" left the Podunk junction.\n");
        lock_release(PRINT); /* release lock so that other can print */
        count--;
}


/*
 *  *  * createcars()
 *   *   *
 *    *    * Arguments:
 *     *     *      int nargs: unused.
 *      *      *      char ** args: unused.
 *       *       *
 *        *        * Returns:
 *         *         *      0 on success.
 *          *          *
 *           *           * Notes:
 *            *            *      Driver code to start up the approachintersection() threads.  You are
 *             *             *      free to modify this code as necessary for your solution.
 *              *              */

int
createcars(int nargs,char ** args)
{
        int index, error;
        (void) nargs;
        (void) args;

        kprintf("Creating Lock\n");
        NE=lock_create("NORTHEAST");
        SE=lock_create("SOUTHEAST");
        SW=lock_create("SOUTHWEST");
        NW=lock_create("NORTHWEST");
        PRINT=lock_create("PRINTINGLOCK");
        LL=lock_create("LEFT_LOCK");
        SL=lock_create("STRAIGHT_LOCK");
       
        for (index = 0; index < NCARS; index++) 
        {

                error = thread_fork("approachintersection thread",
                                    NULL,
                                    index,
                                    approachintersection,
                                    NULL
                                    );
              

                if (error) 
                {
                     panic("approachintersection: thread_fork failed: %s\n",strerror(error));
                
                }
        }


        while(count);
            
        lock_destroy(NE);
        lock_destroy(SE);
        lock_destroy(SW);
        lock_destroy(NW);
        lock_destroy(PRINT);
        lock_destroy(LL);
        lock_destroy(SL);
        kprintf("Done! All cars passed!\n");
        return 0;
}


