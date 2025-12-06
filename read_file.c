/*
** Program to read a file from a QFS disk image
**
** Usage: read_file <disk image file> <file to read> <output file>
**
** Authors: [İsimlerinizi buraya yazın]
** Date: December 2025
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "qfs.h"

int main(int argc, char *argv[]) {
   if (argc != 4) {
       fprintf(stderr, "Usage: %s <disk image file> <file to read> <output file>\n", argv[0]);
       return 1;
   }

   FILE *fp = fopen(argv[1], "rb");
   if (!fp) {
       perror("fopen");
       return 2;
   }

#ifdef DEBUG
   printf("Opened disk image: %s\n", argv[1]);
#endif

   // Read superblock
   superblock_t sb;
   fseek(fp, 0, SEEK_SET);
   if (fread(&sb, sizeof(superblock_t), 1, fp) != 1) {
       fprintf(stderr, "Error: Failed to read superblock\n");
       fclose(fp);
       return 3;
   }

   // Verify QFS magic number
   if (sb.fs_type != 0x51) {
       fprintf(stderr, "Error: Not a valid QFS filesystem\n");
       fclose(fp);
       return 4;
   }

#ifdef DEBUG
   printf("Block size: %d bytes\n", sb.bytes_per_block);
#endif

   // Search for the file in directory entries
   fseek(fp, sizeof(superblock_t), SEEK_SET);
   direntry_t entry;
   int found = 0;
   
   for (int i = 0; i < sb.total_direntries; i++) {
       if (fread(&entry, sizeof(direntry_t), 1, fp) != 1) {
           break;
       }
       
       if (strcmp(entry.filename, argv[2]) == 0) {
           found = 1;
           break;
       }
   }
   
   if (!found) {
       fprintf(stderr, "Error: File '%s' not found in disk image\n", argv[2]);
       fclose(fp);
       return 5;
   }

#ifdef DEBUG
   printf("Found file: %s (size: %u bytes, starting block: %u)\n", 
          entry.filename, entry.file_size, entry.starting_block);
#endif

   // Open output file
   FILE *out_fp = fopen(argv[3], "wb");
   if (!out_fp) {
       perror("fopen output file");
       fclose(fp);
       return 6;
   }

   // Read file data from blocks
   uint32_t bytes_remaining = entry.file_size;
   uint16_t current_block = entry.starting_block;
   uint8_t *buffer = malloc(sb.bytes_per_block);
   
   if (!buffer) {
       fprintf(stderr, "Error: Memory allocation failed\n");
       fclose(fp);
       fclose(out_fp);
       return 7;
   }

   while (bytes_remaining > 0) {
       // Calculate block offset: 8192 (superblock + dir entries) + block_num * block_size
       long block_offset = 8192 + (current_block * sb.bytes_per_block);
       fseek(fp, block_offset, SEEK_SET);
       
       // Read the entire block
       if (fread(buffer, sb.bytes_per_block, 1, fp) != 1) {
           fprintf(stderr, "Error: Failed to read block %u\n", current_block);
           free(buffer);
           fclose(fp);
           fclose(out_fp);
           return 8;
       }

#ifdef DEBUG
       printf("Reading block %u at offset %ld\n", current_block, block_offset);
#endif

       // Calculate how many data bytes are in this block
       // Data starts at byte 1 (after is_busy byte)
       // Last 2 bytes are next_block pointer (if not last block)
       uint32_t data_size = sb.bytes_per_block - 3; // 1 byte is_busy + 2 bytes next_block
       
       if (bytes_remaining < data_size) {
           data_size = bytes_remaining;
       }
       
       // Write data to output file (skip first byte which is is_busy)
       if (fwrite(&buffer[1], 1, data_size, out_fp) != data_size) {
           fprintf(stderr, "Error: Failed to write to output file\n");
           free(buffer);
           fclose(fp);
           fclose(out_fp);
           return 9;
       }
       
       bytes_remaining -= data_size;
       
       // Get next block number from last 2 bytes
       if (bytes_remaining > 0) {
           uint16_t next_block = buffer[sb.bytes_per_block - 2] | 
                                 (buffer[sb.bytes_per_block - 1] << 8);
           current_block = next_block;

#ifdef DEBUG
           printf("Next block: %u, remaining bytes: %u\n", next_block, bytes_remaining);
#endif
       }
   }

   free(buffer);
   fclose(fp);
   fclose(out_fp);
   
   printf("Successfully extracted '%s' to '%s'\n", argv[2], argv[3]);
   return 0;
}