/*
 *  ioctl_impl.c - the process to use ioctl's to control the kernel module
 *
 *  Until now we could have used cat for input and output.  But now
 *  we need to do ioctl's, which require writing our own process.
 * 
 *  This corresponds to the userland file.
 * 
 * First, write to the dev file:
 *              $ echo "Hello from the user" > /dev/mychardev-1
 * 
 * Compile and use:
 *              $ gcc -Wall ioctl_impl.c -o ioctl_impl
 *              $ sudo ./ioctl_impl <nth_byte>
 */

/* 
 * device specifics, such as ioctl numbers and the
 * major device file. 
 */
#include "chardev_impl.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>              /* open */
#include <unistd.h>             /* exit */
#include <sys/ioctl.h>          /* ioctl */

ioctl_get_nth_byte(int file_desc, int ioctl_param)
{
  int i;
  char c;

  printf("get_nth_byte message:");

  i = 0;
  while (i <= ioctl_param) {

    c = ioctl(file_desc, IOCTL_GET_NTH_BYTE, i++);

    if (c < 0) {
      printf("ioctl_get_nth_byte failed at the %d'th byte:\n", i);
      exit(-1);
    }
  }
  putchar(c); // prints the nth byte
  putchar('\n');
}

/* 
 * Main - Call the ioctl functions 
 */
main(int argc, char *argv[])
{
    int file_desc, ret_val;
    char *msg = "Message passed by ioctl\n";

    file_desc = open("/dev/mychardev-1", 0);
    if (file_desc < 0) {
        printf("Can't open device file: %s\n", "/dev/mychardev-1");
        exit(-1);
    }
    
    // transform input string into integer
    char *arg = argv[1];
    int num = atoi(arg);

    ioctl_get_nth_byte(file_desc, num);

    close(file_desc);
}