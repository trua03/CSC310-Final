/*
** Program to recover deleted JPG files from a QFS disk image
**
** Usage: recover_files <disk image file>
**
** This program searches for JPG file signatures (0xFF 0xD8) and 
** reconstructs files until the end marker (0xFF 0xD9) is found.
**
** Authors: [İsimlerinizi buraya yazın]
** Date: December 2025
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "qfs.h"

// JPG file signatures
#define JPG_START_MARKER_1 0xFF
#define JPG_START_MARKER_2 0xD8
#define JPG_END_MARKER_1   0xFF
#define JPG_END_MARKER_2   0xD9

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
   printf("Total blocks: %d\n", sb.total_blocks);
#endif

   // Calculate data blocks start offset
   long data_start = 8192; // sizeof(superblock) + sizeof(dir_entries)
   
   // Get total file size
   fseek(fp, 0, SEEK_END);
   long file_size = ftell(fp);
   long data_area_size = file_size - data_start;

#ifdef DEBUG
   printf("Data area starts at: %ld\n", data_start);
   printf("Data area size: %ld bytes\n", data_area_size);
#endif

   // Read entire data area into memory
   uint8_t *data = malloc(data_area_size);
   if (!data) {
       fprintf(stderr, "Error: Memory allocation failed\n");
       fclose(fp);
       return 5;
   }

   fseek(fp, data_start, SEEK_SET);
   if (fread(data, 1, data_area_size, fp) != (size_t)data_area_size) {
       fprintf(stderr, "Error: Failed to read data area\n");
       free(data);
       fclose(fp);
       return 6;
   }

   fclose(fp);

   // Search for JPG files
   int file_count = 0;
   long i = 0;
   
   while (i < data_area_size - 1) {
       // Look for JPG start marker (0xFF 0xD8)
       if (data[i] == JPG_START_MARKER_1 && data[i + 1] == JPG_START_MARKER_2) {
#ifdef DEBUG
           printf("\nFound JPG start marker at offset %ld\n", i);
#endif
           
           // Found start of JPG, now find the end
           long start_pos = i;
           long j = i + 2;
           int found_end = 0;
           
           // Search for end marker (0xFF 0xD9)
           while (j < data_area_size - 1) {
               if (data[j] == JPG_END_MARKER_1 && data[j + 1] == JPG_END_MARKER_2) {
                   found_end = 1;
                   j += 2; // Include the end marker
                   break;
               }
               j++;
           }
           
           if (found_end) {
               long jpg_size = j - start_pos;
               
#ifdef DEBUG
               printf("Found JPG end marker at offset %ld\n", j - 2);
               printf("JPG size: %ld bytes\n", jpg_size);
#endif
               
               // Create output filename
               char filename[256];
               sprintf(filename, "recovered_file_%d.jpg", file_count + 1);
               
               // Write recovered file
               FILE *out_fp = fopen(filename, "wb");
               if (out_fp) {
                   if (fwrite(&data[start_pos], 1, jpg_size, out_fp) == (size_t)jpg_size) {
                       printf("Recovered: %s (%ld bytes)\n", filename, jpg_size);
                       file_count++;
                   } else {
                       fprintf(stderr, "Error: Failed to write %s\n", filename);
                   }
                   fclose(out_fp);
               } else {
                   fprintf(stderr, "Error: Failed to create %s\n", filename);
               }
               
               // Move past this JPG file
               i = j;
           } else {
#ifdef DEBUG
               printf("No end marker found for JPG starting at %ld\n", start_pos);
#endif
               i++;
           }
       } else {
           i++;
       }
   }

   free(data);
   
   if (file_count == 0) {
       printf("No JPG files recovered.\n");
   } else {
       printf("\nTotal files recovered: %d\n", file_count);
   }

   return 0;
}