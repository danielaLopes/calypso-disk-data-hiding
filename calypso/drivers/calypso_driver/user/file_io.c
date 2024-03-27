/*
 * file_io.c - User space program for random access to blocks 
 *              in file in path CALYPSO_FILE_PATH2 with
 *              sample use in main()
 * 
 * Usage:
 *      $ gcc -Wall file_io.c -o file_io.o
 *      $ ./file_io.o
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "../global.h"

#define CALYPSO_FILE_PATH2 "/home/daniela/vboxshare/thesis/calypso/driver/calypso2.dat"

#define BLOCK_SIZE 4096


size_t read_block_from_file(int calypso_file, 
                            loff_t offset_bytes, 
                            size_t len_bytes,
                            char *req_buf)
{
    size_t len;

    printf("offset_bytes: %ld\n", offset_bytes);

    lseek(calypso_file, offset_bytes, SEEK_SET);
    len = read(calypso_file, req_buf, len_bytes);

    printf("Read data: %s\n", req_buf);

    return len;
} 

size_t write_block_to_file(int calypso_file, 
                            loff_t offset_bytes, 
                            size_t len_bytes,
                            char *req_buf) 
{
    size_t len;

    printf("offset_bytes: %ld\n", offset_bytes);

    lseek(calypso_file, offset_bytes, SEEK_SET);
    len = write(calypso_file, req_buf, len_bytes);

    return len;
}


int main() 
{
    int calypso_file_read;
    int calypso_file_write = open(CALYPSO_FILE_PATH2, O_WRONLY | O_CREAT, 0);
    if (calypso_file_write < 0) 
    {
        printf("could not open calypso_file_write\n");
        return -1;
    }
    printf("opened calypso_file_write\n");

    char *read_buf = malloc(BLOCK_SIZE * sizeof(char));
    char **write_bufs = malloc(14 * sizeof(char*));
    for(int i = 0; i < 10; i++) 
    {
        write_bufs[i] = malloc(BLOCK_SIZE * sizeof(char));
        sprintf(write_bufs[i], "block #%d", i);
    }
    write_bufs[10] = malloc(BLOCK_SIZE * sizeof(char));
    write_bufs[11] = malloc(BLOCK_SIZE * sizeof(char));
    write_bufs[12] = malloc(BLOCK_SIZE * sizeof(char));
    write_bufs[13] = malloc(BLOCK_SIZE * sizeof(char));
   
    sprintf(write_bufs[10], "block #%d", 20);
    sprintf(write_bufs[11], "block #%d", 30);
    sprintf(write_bufs[12], "block #%d", 40);
    sprintf(write_bufs[13], "block #%d", 100);

    for(int i = 0; i < 10; i++) 
    {
        printf("Write block %d: %lu\n", i, write_block_to_file(calypso_file_write, i * BLOCK_SIZE, BLOCK_SIZE, write_bufs[i]));
    }
    // random-access blocks - WRITE
    printf("Write block %d: %lu\n", 5, write_block_to_file(calypso_file_write, 5 * BLOCK_SIZE, BLOCK_SIZE, write_bufs[8]));
    printf("Write block %d: %lu\n", 20, write_block_to_file(calypso_file_write, 20 * BLOCK_SIZE, BLOCK_SIZE, write_bufs[10]));
    printf("Write block %d: %lu\n", 100, write_block_to_file(calypso_file_write, 100 * BLOCK_SIZE, BLOCK_SIZE, write_bufs[13]));
    printf("Write block %d: %lu\n", 30, write_block_to_file(calypso_file_write, 30 * BLOCK_SIZE, BLOCK_SIZE, write_bufs[11]));
    printf("Write block %d: %lu\n", 40, write_block_to_file(calypso_file_write, 40 * BLOCK_SIZE, BLOCK_SIZE, write_bufs[12]));

    close(calypso_file_write);


    calypso_file_read = open(CALYPSO_FILE_PATH2, O_RDONLY);
    if (calypso_file_read < 0) {
        printf("could not open calypso_file_read\n");
        return -1;
    }
    printf("opened calypso_file_read\n");

    for(int i = 0; i < 10; i++) 
    {
        printf("Read block %d: %lu\n", i, read_block_from_file(calypso_file_read, i * BLOCK_SIZE, BLOCK_SIZE, write_bufs[i]));
    }
    // random-access blocks - READ
    printf("Read block %d: %lu\n", 5, read_block_from_file(calypso_file_write, 5 * BLOCK_SIZE, BLOCK_SIZE, write_bufs[8]));
    printf("Read block %d: %lu\n", 20, read_block_from_file(calypso_file_write, 20 * BLOCK_SIZE, BLOCK_SIZE, write_bufs[10]));
    printf("Read block %d: %lu\n", 100, read_block_from_file(calypso_file_write, 100 * BLOCK_SIZE, BLOCK_SIZE, write_bufs[13]));
    printf("Read block %d: %lu\n", 30, read_block_from_file(calypso_file_write, 30 * BLOCK_SIZE, BLOCK_SIZE, write_bufs[11]));
    printf("Read block %d: %lu\n", 40, read_block_from_file(calypso_file_write, 40 * BLOCK_SIZE, BLOCK_SIZE, write_bufs[12]));

    close(calypso_file_read);

    for(int i = 0; i < 14; i++) 
    {
        free(write_bufs[i]);
    }
    free(write_bufs);
    free(read_buf);
}
