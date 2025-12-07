/*
** Program to read a file from a QFS disk image
**
** Usage: read_file <disk image> <file to read> <output file>
**
** Reads a file from the QFS disk by following the block chain
** and writes it to the local filesystem.
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

   // Verify QFS
   if (sb.fs_type != 0x51) {
       fprintf(stderr, "Error: Not a valid QFS filesystem\n");
       fclose(fp);
       return 4;
   }

#ifdef DEBUG
   printf("Block size: %d bytes\n", sb.bytes_per_block);
#endif

   // Search for file in directory entries
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
       fprintf(stderr, "Error: File '%s' not found\n", argv[2]);
       fclose(fp);
       return 5;
   }

#ifdef DEBUG
   printf("Found file: %s\n", entry.filename);
   printf("  Size: %u bytes\n", entry.file_size);
   printf("  Starting block: %u\n", entry.starting_block);
#endif

   // Open output file
   FILE *out = fopen(argv[3], "wb");
   if (!out) {
       perror("fopen output");
       fclose(fp);
       return 6;
   }

   // Allocate buffer for reading blocks
   uint8_t *buffer = malloc(sb.bytes_per_block);
   if (!buffer) {
       fprintf(stderr, "Error: Memory allocation failed\n");
       fclose(fp);
       fclose(out);
       return 7;
   }

   // Read file data following block chain
   uint32_t bytes_left = entry.file_size;
   uint16_t current_block = entry.starting_block;
   
   while (bytes_left > 0) {
       // Calculate block offset: 8192 + (block_num * block_size)
       long offset = 8192 + ((long)current_block * sb.bytes_per_block);
       fseek(fp, offset, SEEK_SET);
       
       // Read entire block
       if (fread(buffer, sb.bytes_per_block, 1, fp) != 1) {
           fprintf(stderr, "Error: Failed to read block %u\n", current_block);
           free(buffer);
           fclose(fp);
           fclose(out);
           return 8;
       }

#ifdef DEBUG
       printf("Reading block %u at offset %ld\n", current_block, offset);
#endif

       // Calculate data size in this block
       // Block structure: [is_busy(1)][data(n-3)][next_block(2)]
       uint32_t data_in_block = sb.bytes_per_block - 3;
       
       if (bytes_left < data_in_block) {
           data_in_block = bytes_left;
       }
       
       // Write data (skip first byte, which is is_busy)
       if (fwrite(&buffer[1], 1, data_in_block, out) != data_in_block) {
           fprintf(stderr, "Error: Failed to write output\n");
           free(buffer);
           fclose(fp);
           fclose(out);
           return 9;
       }
       
       bytes_left -= data_in_block;
       
       // Get next block from last 2 bytes (little-endian)
       if (bytes_left > 0) {
           current_block = buffer[sb.bytes_per_block - 2] | 
                          (buffer[sb.bytes_per_block - 1] << 8);

#ifdef DEBUG
           printf("Next block: %u, bytes remaining: %u\n", current_block, bytes_left);
#endif
       }
   }

   free(buffer);
   fclose(fp);
   fclose(out);
   
   printf("File '%s' extracted to '%s'\n", argv[2], argv[3]);
   return 0;
}