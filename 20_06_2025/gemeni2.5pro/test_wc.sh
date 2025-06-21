#!/bin/bash

# test_wc.sh
# Integration tests for fast_wc

TARGET=./fast_wc
SYS_WC=/usr/bin/wc

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo "Building the target..."
make fast_wc || { echo "Build failed!"; exit 1; }

# Function to run a test and check its output
run_test() {
    local test_name="$1"
    local file="$2"
    
    # Run our wc and system wc, normalize whitespace for comparison
    output_ours=$( $TARGET "$file" | tr -s ' ' )
    output_sys=$( $SYS_WC "$file" | tr -s ' ' )

    if [ "$output_ours" == "$output_sys" ]; then
        printf "[  ${GREEN}PASS${NC}  ] %s\n" "$test_name"
    else
        printf "[  ${RED}FAIL${NC}  ] %s\n" "$test_name"
        echo "    OURS: '$output_ours'"
        echo "    SYS : '$output_sys'"
        exit 1
    fi
}

# --- Test Cases ---

# 1. Empty file
echo -n "" > empty.txt
run_test "Empty File" empty.txt

# 2. Single line with newline
echo "hello world" > oneline_nl.txt
run_test "Single Line with NL" oneline_nl.txt

# 3. Single line without newline
echo -n "hello world" > oneline_no_nl.txt
run_test "Single Line without NL" oneline_no_nl.txt

# 4. Multiple lines and complex spacing
printf "  leading space\nmiddle \t tab\n trailing space \n\n" > complex.txt
run_test "Complex Spacing" complex.txt

# 5. File larger than buffer size (128KB)
# Create a 1MB file of random-ish text
head -c 1000 /dev/urandom | base64 > large_file.txt
head -c 1M large_file.txt > large_file_1M.txt
run_test "Large File (1MB)" large_file_1M.txt

# 6. Test stdin
test_stdin_name="STDIN Pipe"
output_ours_stdin=$(cat complex.txt | $TARGET | tr -s ' ')
output_sys_stdin=$(cat complex.txt | $SYS_WC | tr -s ' ')
if [ "$output_ours_stdin" == "$output_sys_stdin" ]; then
    printf "[  ${GREEN}PASS${NC}  ] %s\n" "$test_stdin_name"
else
    printf "[  ${RED}FAIL${NC}  ] %s\n" "$test_stdin_name"
    echo "    OURS: '$output_ours_stdin'"
    echo "    SYS : '$output_sys_stdin'"
    exit 1
fi

# 7. Test multiple files
test_multi_name="Multiple Files"
output_ours_multi=$( $TARGET empty.txt oneline_nl.txt complex.txt | tr -s ' ' )
output_sys_multi=$( $SYS_WC empty.txt oneline_nl.txt complex.txt | tr -s ' ' )
if [ "$output_ours_multi" == "$output_sys_multi" ]; then
    printf "[  ${GREEN}PASS${NC}  ] %s\n" "$test_multi_name"
else
    printf "[  ${RED}FAIL${NC}  ] %s\n" "$test_multi_name"
    echo "    OURS: '$output_ours_multi'"
    echo "    SYS : '$output_sys_multi'"
    exit 1
fi


# Cleanup
rm -f empty.txt oneline_nl.txt oneline_no_nl.txt complex.txt large_file.txt large_file_1M.txt

echo -e "\n${GREEN}All integration tests passed successfully!${NC}"
