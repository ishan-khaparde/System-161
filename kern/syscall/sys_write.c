#include <types.h>
#include <lib.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <kern/limits.h>
#include <machine/trapframe.h>

int sys_write(struct trapframe *tf) {
     char *buf;
     size_t i;
     int len;

     len = tf->tf_a2;
     buf = (char *)kmalloc(len);
     copyin(tf->tf_a1, buf, len);

     for (i=0; i<len; i++) {
                putch(buf[i]);
     }

     kfree(buf);
     return 0;
}
