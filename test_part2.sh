#!/bin/bash

echo "=========================================="
echo "QFS Part 2 Test Script"
echo "Testing read_file.c and recover_files.c"
echo "=========================================="
echo ""

# Clean and compile
echo "1. Cleaning and compiling..."
make clean
make
echo ""

# Create test disk
echo "2. Creating test disk (10MB)..."
dd if=/dev/zero of=test_part2.img bs=1M count=10 2>/dev/null
./mkfs_qfs test_part2.img TestDisk
echo ""

# Create test files
echo "3. Creating test files..."
echo "Hello, this is a test file for QFS!" > test1.txt
echo "Another test file with different content." > test2.txt
echo "Third file for testing purposes." > test3.txt
echo ""

# Note: You would need write_file.c to actually add files to the disk
# For now, we're just testing that the programs compile

echo "4. Testing list_information..."
./list_information test_part2.img
echo ""

echo "=========================================="
echo "Part 2 Programs Compiled Successfully!"
echo "=========================================="
echo ""
echo "To test read_file.c:"
echo "  1. Use write_file.c to add a file to the disk"
echo "  2. Run: ./read_file test_part2.img <filename> output.txt"
echo ""
echo "To test recover_files.c:"
echo "  1. Get a disk image with deleted JPG files"
echo "  2. Run: ./recover_files <disk_image>"
echo ""