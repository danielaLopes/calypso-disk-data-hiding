//#include <sysdep.h>

/*
To compile and run:
gcc make_write.c -o make_write.o
./make_write.o
*/ 
/* 
Example from https://sys.readthedocs.io/en/latest/doc/07_calling_system_calls.html
*/
#include <fcntl.h>
#include <unistd.h>

int main ()
{
    //write (1, "Hello World\n", 11); // 1 is to write to console
    syscall (1, 1, "Hello World\n", 11);

    return 0;
}