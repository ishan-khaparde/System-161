#include <types.h>
#include <lib.h>
#include <thread.h>
#include <curthread.h>

void sys__exit(int exitcode) {
        thread_exit();

}
