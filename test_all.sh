#!/bin/bash

echo "=========================================="
echo "QFS Full Test Suite"
echo "=========================================="
echo ""

# pic_test klasörüne git
cd pic_test

echo "1. Testing list_information on qu_pics.img"
echo "=========================================="
../list_information qu_pics.img
echo ""

echo "2. Testing read_file - Extracting all JPGs"
echo "=========================================="
for i in {1..10}; do
    echo "  Extracting pic$i.jpg..."
    ../read_file qu_pics.img pic$i.jpg extracted_pic$i.jpg
done
echo ""

echo "3. Verifying extracted files"
echo "=========================================="
ls -lh extracted_pic*.jpg
echo ""
echo "Total extracted: $(ls extracted_pic*.jpg 2>/dev/null | wc -l) files"
echo ""

echo "4. Testing recover_files on qu_pics_reformatted1.img"
echo "=========================================="
../recover_files qu_pics_reformatted1.img
echo ""

echo "5. Recovered files from reformatted1:"
echo "=========================================="
ls -lh recovered_file_*.jpg 2>/dev/null
echo "Total recovered: $(ls recovered_file_*.jpg 2>/dev/null | wc -l) files"
echo ""

# Clean up recovered files
rm -f recovered_file_*.jpg

echo "6. Testing recover_files on qu_pics_reformatted2.img"
echo "=========================================="
../recover_files qu_pics_reformatted2.img
echo ""

echo "7. Recovered files from reformatted2:"
echo "=========================================="
ls -lh recovered_file_*.jpg 2>/dev/null
echo "Total recovered: $(ls recovered_file_*.jpg 2>/dev/null | wc -l) files"
echo ""

echo "8. Comparing original vs recovered (first file)"
echo "=========================================="
if [ -f pic1.jpg ] && [ -f recovered_file_1.jpg ]; then
    echo "Original pic1.jpg size: $(ls -lh pic1.jpg | awk '{print $5}')"
    echo "Recovered file size: $(ls -lh recovered_file_1.jpg | awk '{print $5}')"
    
    if diff -q pic1.jpg recovered_file_1.jpg > /dev/null 2>&1; then
        echo "✅ Files are IDENTICAL!"
    else
        echo "⚠️  Files are different"
    fi
else
    echo "Cannot compare - files not found"
fi
echo ""

echo "=========================================="
echo "✅ All tests completed!"
echo "=========================================="
echo ""
echo "Summary:"
echo "  - list_information: Check output above"
echo "  - read_file: $(ls extracted_pic*.jpg 2>/dev/null | wc -l) files extracted"
echo "  - recover_files: $(ls recovered_file_*.jpg 2>/dev/null | wc -l) files recovered"
echo ""