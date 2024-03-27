//#include <sysdep.h>

/* 
https://sys.readthedocs.io/en/latest/doc/07_calling_system_calls.html
glibc provides a wrapper around system calls using function calls
*/

// to know the syscall id: $ cat /usr/include/asm-generic/unistd.h | grep __NR_read
// which is defined in fs/read_write.c
// or go to the manual page in https://man7.org/linux/man-pages/man2/syscalls.2.html
//#define __NR_read 63

//int _read(int status)
//{
    // ssize_t read(int fd, void *buf, size_t count);
    // read() attempts to read up to count bytes from file descriptor fd
    // into the buffer starting at buf
    //return INLINE_SYSCALL(read, 1, status); // takes 1 parameter "status"
//}


/*
To compile and run:
gcc make_read.c -o make_read.o
./make_read.o
*/ 
/* 
Example from https://linux.die.net/man/3/read
*/
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

int main ()
{
    char buf[20];
    size_t nbytes;
    ssize_t bytes_read;
    int fd;

    nbytes = sizeof(buf);
    bytes_read = read(fd, buf, nbytes);

    return 0;
}