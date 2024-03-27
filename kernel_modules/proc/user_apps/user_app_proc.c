#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
 
void main(void)
{
	char buf[100];
	int fd = open("/proc/helloworld", O_RDWR);
    //int fd = open("/proc/helloworld", O_RDONLY);
	read(fd, buf, 100);
	puts(buf);
 
	lseek(fd, 0 , SEEK_SET); // move position back to 0
	write(fd, "test", 5);
	
	lseek(fd, 0 , SEEK_SET); // move position back to 0
	read(fd, buf, 100);
	puts(buf);
}