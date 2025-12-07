/*
** Program to recover deleted JPG files from a QFS disk image
**
** Usage: recover_files <disk image>
**
** Simple, reliable approach: Extract clean data, find JPG signatures
**
** Recovered files are saved as recovered_file_1.jpg, recovered_file_2.jpg, etc.
**
** Authors: [Your names here]
** Date: December 2025
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "qfs.h"

#define JPG_START_1 0xFF
#define JPG_START_2 0xD8
#define JPG_END_1   0xFF
#define JPG_END_2   0xD9

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filesystem_image>\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        perror("fopen");
        return 2;
    }

    // Read superblock
    superblock_t sb;
    fseek(fp, 0, SEEK_SET);
    if (fread(&sb, sizeof(superblock_t), 1, fp) != 1) {
        fprintf(stderr, "Error: Failed to read superblock\n");
        fclose(fp);
        return 3;
    }

    if (sb.fs_type != 0x51) {
        fprintf(stderr, "Error: Not a valid QFS filesystem\n");
        fclose(fp);
        return 4;
    }

    // Extract clean data (remove block metadata)
    long data_start = 8192;
    int data_per_block = sb.bytes_per_block - 3; // Skip is_busy(1) and next_block(2)
    long clean_size = (long)sb.total_blocks * data_per_block;

    uint8_t *clean_data = malloc(clean_size);
    uint8_t *block_buf = malloc(sb.bytes_per_block);
    
    if (!clean_data || !block_buf) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        if (clean_data) free(clean_data);
        if (block_buf) free(block_buf);
        fclose(fp);
        return 5;
    }

    long clean_pos = 0;
    for (int i = 0; i < sb.total_blocks; i++) {
        long block_offset = data_start + ((long)i * sb.bytes_per_block);
        fseek(fp, block_offset, SEEK_SET);
        
        if (fread(block_buf, sb.bytes_per_block, 1, fp) == 1) {
            // Copy only data portion: skip byte 0 and last 2 bytes
            memcpy(&clean_data[clean_pos], &block_buf[1], data_per_block);
            clean_pos += data_per_block;
        }
    }

    free(block_buf);
    fclose(fp);

    printf("Scanning for JPG files...\n");

    int file_count = 0;
    long pos = 0;

    while (pos < clean_pos - 1) {
        // Look for JPG start (0xFF 0xD8)
        if (clean_data[pos] == JPG_START_1 && clean_data[pos + 1] == JPG_START_2) {
            long start = pos;
            long end = -1;
            
            // Find FIRST end marker within 2MB
            long search_limit = pos + 2000000;
            if (search_limit > clean_pos - 1) search_limit = clean_pos - 1;
            
            for (long scan = pos + 2; scan < search_limit; scan++) {
                if (clean_data[scan] == JPG_END_1 && clean_data[scan + 1] == JPG_END_2) {
                    end = scan + 2;
                    break;
                }
            }
            
            if (end > 0) {
                long jpg_size = end - start;
                
                // Reasonable size check
                if (jpg_size >= 1000 && jpg_size <= 2000000) {
                    char filename[64];
                    sprintf(filename, "recovered_file_%d.jpg", file_count + 1);
                    
                    FILE *out = fopen(filename, "wb");
                    if (out) {
                        if (fwrite(&clean_data[start], 1, jpg_size, out) == (size_t)jpg_size) {
                            printf("Recovered: %s (%ld bytes)\n", filename, jpg_size);
                            file_count++;
                        }
                        fclose(out);
                    }
                }
                pos = end;
            } else {
                pos++;
            }
        } else {
            pos++;
        }
    }

    free(clean_data);
    
    printf("\n");
    if (file_count == 0) {
        printf("No JPG files recovered.\n");
    } else {
        printf("Successfully recovered %d JPG file(s).\n", file_count);
    }

    return 0;
}
