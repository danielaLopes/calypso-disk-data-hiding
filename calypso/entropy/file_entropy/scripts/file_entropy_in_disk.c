#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <libgen.h>

#include "../../disk_entropy.h"


#define BLOCKS_IN_A_GB 1073741824 / 4096

// $ gcc -Wall file_entropy_in_disk.c ../../disk_entropy.c -o file_entropy_in_disk -lm
// $ sudo ./file_entropy_in_disk
int main(int argc, char** argv)
{
    unsigned long block_index;
    unsigned char block[4096 + 1];

    double entropy;
    /* 5 decimal houses + 1 + 1 + '\n' + null termination */
    char output_entropy[9];

    char full_results_path[1000];
    // argv[0] contains the full path to this executable
    strcpy(full_results_path, dirname(argv[0]));
    // the path still maintains the '.' from executing the executable
    strcat(full_results_path, "/");
    // full_results_path[strlen(full_results_path)-1] = '\0';
    strcat(full_results_path, argv[1]);
    printf("Writing results to %s\n", full_results_path);
    FILE *results = fopen(full_results_path, "w");
    if (!results)
    {
        printf("Error opening results\n");
        return -1;
    }

    /* We need to read the file /dev/sdb block by block, 
    because it may not fit all in memory */
    printf("Plaintext disk : %s\n", argv[2]);
    fwrite("PLAINTEXT\n", 1, 10, results);
    FILE *plaintext_disk = fopen(argv[2], "rb");
    if (!plaintext_disk)
    {
        printf("Error opening plaintext %s\n", argv[2]);
        return -1;
    }
    /**
     * Read file_data to know which files and blocks to classify
     * Write the results of the entropy for each block in a results file 
     */
    for (block_index = 0; block_index < BLOCKS_IN_A_GB; block_index++)
    {
        fread(block, 1, 4096, plaintext_disk);
        entropy = shannon_entropy(block);

        snprintf(output_entropy, 9, "%.5f\n", entropy);
        fwrite(output_entropy, 1, 8, results);
    }
    fclose(plaintext_disk);

    printf("Image disk : %s\n", argv[3]);
    fwrite("IMAGE\n", 1, 6, results);
    FILE *image_disk = fopen(argv[3], "rb");
    if (!image_disk)
    {
        printf("Error opening image disk %s\n", argv[3]);
        return -1;
    }
    for (block_index = 0; block_index < BLOCKS_IN_A_GB; block_index++)
    {
        fread(block, 1, 4096, image_disk);
        entropy = shannon_entropy(block);

        snprintf(output_entropy, 9, "%.5f\n", entropy);
        fwrite(output_entropy, 1, 8, results);
    }
    fclose(image_disk);

    // printf("Video disk\n");
    // FILE *video_disk = fopen("/dev/sdd", "rb");
    // if (!video_disk)
    // {
    //     printf("Error opening video disk /dev/sdd");
    //     return -1;
    // }
    // for (block_index = 0; block_index < BLOCKS_IN_A_GB; block_index++)
    // {
    //     fread(block, 1, 4096, video_disk);
    //     entropy = shannon_entropy(block);

    //     snprintf(output_entropy, 9, "%.5f\n", entropy);
    //     fwrite(output_entropy, 1, 8, results);
    // }
    // fclose(video_disk);

    printf("Compressed disk : %s\n", argv[4]);
    fwrite("COMPRESSED\n", 1, 11, results);
    // FILE *compressed_disk = fopen("/dev/sde", "rb");
    FILE *compressed_disk = fopen(argv[4], "rb");
    if (!compressed_disk)
    {
        printf("Error opening compressed disk %s", argv[4]);
        return -1;
    }
    for (block_index = 0; block_index < BLOCKS_IN_A_GB; block_index++)
    {
        fread(block, 1, 4096, compressed_disk);
        entropy = shannon_entropy(block);

        snprintf(output_entropy, 9, "%.5f\n", entropy);
        fwrite(output_entropy, 1, 8, results);
    }
    fclose(compressed_disk);

    printf("Encrypted disk : %s\n", argv[5]);
    fwrite("ENCRYPTED\n", 1, 10, results);
    // FILE *encrypted_disk = fopen("/dev/sdf", "rb");
    FILE *encrypted_disk = fopen(argv[5], "rb");
    if (!encrypted_disk)
    {
        printf("Error opening encrypted disk %s", argv[5]);
        return -1;
    }
    for (block_index = 0; block_index < BLOCKS_IN_A_GB; block_index++)
    {
        fread(block, 1, 4096, encrypted_disk);
        entropy = shannon_entropy(block);

        snprintf(output_entropy, 9, "%.5f\n", entropy);
        fwrite(output_entropy, 1, 8, results);
    }
    fclose(encrypted_disk);

    fclose(results);

    return 0;
}