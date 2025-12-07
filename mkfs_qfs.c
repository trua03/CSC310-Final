/*
** Program to make a filesystem on a blank file using the qfs parameters
** Authors: Tuana Turhan and Thomas Rua
** Date: December 2025
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "qfs.h"

int main(int argc, char *argv[]) {

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s <disk image file> [<label>]\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "rb+");
    if (!fp) {
        perror("fopen");
        return 2;
    }

#ifdef DEBUG
    fprintf(stderr,"Opened disk image: %s\n", argv[1]);
#endif

    // Initialize superblock structure
    superblock_t sb;
    memset(&sb, 0, sizeof(superblock_t));
    sb.fs_type = 0x51; // QFS magic number

    // Set label if provided
    if (argc == 3) {
        strncpy((char *)sb.label, argv[2], sizeof(sb.label) - 1);
        sb.label[sizeof(sb.label) - 1] = '\0';

#ifdef DEBUG
        fprintf(stderr,"Label: %s\n", argv[2]);
#endif
    }

    // Determine file size of disk image
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

#ifdef DEBUG
    fprintf(stderr, "File size: %ld bytes\n", file_size);
#endif

    // Calculate total data space available (excluding superblock and directory entries)
    int total_data_available = file_size - sizeof(superblock_t) - (sizeof(direntry_t) * 255);

#ifdef DEBUG
    fprintf(stderr, "Total data available: %d bytes\n", total_data_available);
#endif

    // Determine block size based on total data available
    // size <= 30MB (31457280 bytes): 512 bytes per block
    // 30MB < size <= 60MB (62914560 bytes): 1024 bytes per block
    // 60MB < size <= 120MB (125829120 bytes): 2048 bytes per block
    
    if (total_data_available <= 31457280) {
        sb.bytes_per_block = 512;
    } else if (total_data_available <= 62914560) {
        sb.bytes_per_block = 1024;
    } else {
        sb.bytes_per_block = 2048;
    }

#ifdef DEBUG
    fprintf(stderr, "Block size: %d bytes\n", sb.bytes_per_block);
#endif

    // Calculate total number of blocks
    sb.total_blocks = total_data_available / sb.bytes_per_block;
    sb.available_blocks = sb.total_blocks;

#ifdef DEBUG
    fprintf(stderr, "Total blocks: %d\n", sb.total_blocks);
    fprintf(stderr, "Available blocks: %d\n", sb.available_blocks);
#endif

    // Set directory entry counts
    sb.total_direntries = 255;
    sb.available_direntries = sb.total_direntries;

#ifdef DEBUG
    fprintf(stderr, "Total directory entries: %d\n", sb.total_direntries);
    fprintf(stderr, "Available directory entries: %d\n", sb.available_direntries);
#endif

    // Create empty directory entries area
    uint8_t dir_zeros[sizeof(direntry_t) * 255] = {0};

#ifdef DEBUG
    fprintf(stderr, "Size of superblock: %lu bytes\n", sizeof(superblock_t));
    fprintf(stderr, "Size of directory entries area: %lu bytes\n", sizeof(dir_zeros));
    fprintf(stderr, "Data blocks start at byte offset: %lu\n", sizeof(superblock_t) + sizeof(dir_zeros));
#endif

    // Write superblock and directory entries to file
    fwrite(&sb, sizeof(superblock_t), 1, fp);
    fwrite(dir_zeros, sizeof(dir_zeros), 1, fp);

#ifdef DEBUG
    fprintf(stderr,"Marking all data blocks as free...\n");
#endif

    // Mark all data blocks as free (first byte = 0)
    uint8_t free_marker = 0x00;
    fseek(fp, sizeof(superblock_t) + sizeof(dir_zeros), SEEK_SET);
    
    for (int i = 0; i < sb.total_blocks; i++) {
        fwrite(&free_marker, 1, 1, fp);
        // Move to next block (skip remaining bytes in current block)
        fseek(fp, sb.bytes_per_block - 1, SEEK_CUR);
    }

#ifdef DEBUG
    fprintf(stderr, "Filesystem formatted successfully!\n");
#endif

    // Flush and close file
    fflush(fp);
    fclose(fp);

    return 0;
}