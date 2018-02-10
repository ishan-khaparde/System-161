#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
char  *filename = "/testbin/add";
        char  *args[4];
        pid_t  pid;

        argv[0] = "add";
        argv[1] = "780";
        argv[2] = "20";
        argv[3] = NULL;

        pid = fork();
        if (pid == 0) execv(filename, argv);
return 0;
}

