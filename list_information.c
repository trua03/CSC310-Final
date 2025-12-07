/*
** Program to list information about a QFS disk image
** Authors: Tuana Turhan and Thomas Rua
** Date: December 2025
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "qfs.h"

// File type definitions based on bits [6:7] of permissions
#define FILE_TYPE_MASK 0xC0  // 11000000 in binary
#define FILE_TYPE_REGULAR 0x00
#define FILE_TYPE_DIRECTORY 0x40

// Function to get file type string
const char* get_file_type(uint8_t permissions) {
    uint8_t type_bits = permissions & FILE_TYPE_MASK;
    if (type_bits == FILE_TYPE_DIRECTORY) {
        return "Directory";
    }
    return "Regular File";
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <disk image file>\n", argv[0]);
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
        fprintf(stderr, "Error: Not a valid QFS filesystem (magic number: 0x%02X)\n", sb.fs_type);
        fclose(fp);
        return 4;
    }

    // Print superblock information
    printf("=== QFS Disk Information ===\n");
    if (sb.label[0] != '\0') {
        printf("Volume Label: %s\n", sb.label);
    }
    printf("Block Size: %d bytes\n", sb.bytes_per_block);
    printf("Total Blocks: %d\n", sb.total_blocks);
    printf("Free Blocks: %d\n", sb.available_blocks);
    printf("Total Directory Entries: %d\n", sb.total_direntries);
    printf("Free Directory Entries: %d\n", sb.available_direntries);
    printf("\n");

    // Read and display directory entries
    printf("=== Directory Contents ===\n");
    
    // Seek to directory entries area (right after superblock)
    fseek(fp, sizeof(superblock_t), SEEK_SET);
    
    int file_count = 0;
    direntry_t entry;
    
    for (int i = 0; i < sb.total_direntries; i++) {
        if (fread(&entry, sizeof(direntry_t), 1, fp) != 1) {
            fprintf(stderr, "Error: Failed to read directory entry %d\n", i);
            break;
        }
        
        // Check if entry is in use (filename is not empty)
        if (entry.filename[0] != '\0') {
            file_count++;
            printf("File #%d:\n", file_count);
            printf("  Name: %s\n", entry.filename);
            printf("  Size: %u bytes\n", entry.file_size);
            printf("  Type: %s\n", get_file_type(entry.permissions));
            printf("  Starting Block: %u\n", entry.starting_block);
            printf("  Owner ID: %u\n", entry.owner_id);
            printf("  Group ID: %u\n", entry.group_id);
            
            // Display permissions (simplified)
            printf("  Permissions: ");
            uint8_t user_perms = (entry.permissions & 0x03);
            uint8_t group_perms = (entry.permissions >> 2) & 0x03;
            uint8_t world_perms = (entry.permissions >> 4) & 0x03;
            
            // User permissions
            printf((user_perms & 0x02) ? "r" : "-");
            printf((user_perms & 0x01) ? "w" : "-");
            
            // Group permissions
            printf((group_perms & 0x02) ? "r" : "-");
            printf((group_perms & 0x01) ? "w" : "-");
            
            // World permissions
            printf((world_perms & 0x02) ? "r" : "-");
            printf((world_perms & 0x01) ? "w" : "-");
            printf("\n\n");
        }
    }
    
    if (file_count == 0) {
        printf("(No files found)\n");
    } else {
        printf("Total files: %d\n", file_count);
    }

    fclose(fp);
    return 0;
}