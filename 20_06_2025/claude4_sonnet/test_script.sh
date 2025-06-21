#!/bin/bash

# Comprehensive test script for efficient wc utility
# Tests correctness, performance, and edge cases

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test results tracking
TESTS_PASSED=0
TESTS_FAILED=0
TEST_DIR="./test_files"

# Helper functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[PASS]${NC} $1"
    ((TESTS_PASSED++))
}

log_error() {
    echo -e "${RED}[FAIL]${NC} $1"
    ((TESTS_FAILED++))
}

log_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

# Check if our wc exists
check_prerequisites() {
    log_info "Checking prerequisites..."
    
    if [[ ! -f "./wc" ]]; then
        log_error "wc binary not found. Run 'make' first."
        exit 1
    fi
    
    if ! command -v wc >/dev/null 2>&1; then
        log_error "System wc not found"
        exit 1
    fi
    
    # Create test directory
    mkdir -p "$TEST_DIR"
    
    log_success "Prerequisites check passed"
}

# Create test files with various content
create_test_files() {
    log_info "Creating test files..."
    
    # Empty file
    touch "$TEST_DIR/empty.txt"
    
    # Single line without newline
    printf "hello world" > "$TEST_DIR/no_newline.txt"
    
    # Single line with newline
    printf "hello world\n" > "$TEST_DIR/single_line.txt"
    
    # Multiple lines
    printf "line 1\nline 2\nline 3\n" > "$TEST_DIR/multi_line.txt"
    
    # Only whitespace
    printf "   \t\n  \n\n" > "$TEST_DIR/whitespace.txt"
    
    # Only newlines
    printf "\n\n\n\n\n" > "$TEST_DIR/only_newlines.txt"
    
    # Mixed content
    printf "word1 word2\n\nword3   word4\nword5\n" > "$TEST_DIR/mixed.txt"
    
    # Large file
    seq 1 10000 | sed 's/.*/Line & with some additional text content/' > "$TEST_DIR/large.txt"
    
    # Very long lines
    python3 -c "
import sys
print('a' * 5000)
print('b' * 10000 + ' ' + 'c' * 3000)
print('short line')
" > "$TEST_DIR/long_lines.txt"
    
    # Binary-like content
    printf "hello\x00world\ntest\x00\x00line\n" > "$TEST_DIR/binary.txt"
    
    # Unicode content (but still ASCII for our implementation)
    printf "ASCII text\nwith special chars: !@#$%^&*()\ntabs\there\n" > "$TEST_DIR/special_chars.txt"
    
    # File with no final newline but multiple lines
    printf "line1\nline2\nline3" > "$TEST_DIR/no_final_newline.txt"
    
    # Many small words
    printf "a b c d e f g h i j k l m n o p q r s t u v w x y z\n" > "$TEST_DIR/many_words.txt"
    
    # Repeated pattern (good for SIMD testing)
    python3 -c "print('test line\\n' * 1000, end='')" > "$TEST_DIR/repeated.txt"
    
    log_success "Test files created"
}

# Compare outputs between system wc and our wc
compare_outputs() {
    local file="$1"
    local flags="$2"
    local test_name="$3"
    
    # Get outputs
    local system_output=$(wc $flags "$file" 2>/dev/null || echo "ERROR")
    local our_output=$(./wc $flags "$file" 2>/dev/null || echo "ERROR")
    
    if [[ "$system_output" == "$our_output" ]]; then
        log_success "Output match: $test_name"
        return 0
    else
        log_error "Output mismatch: $test_name"
        echo "  System: $system_output"
        echo "  Ours:   $our_output"
        return 1
    fi
}

# Test basic functionality
test_basic_functionality() {
    log_info "Testing basic functionality..."
    
    local files=(
        "empty.txt"
        "no_newline.txt"
        "single_line.txt"
        "multi_line.txt"
        "whitespace.txt"
        "only_newlines.txt"
        "mixed.txt"
        "special_chars.txt"
        "no_final_newline.txt"
        "many_words.txt"
    )
    
    local flags_list=(
        ""      # default (lines, words, bytes)
        "-l"    # lines only
        "-w"    # words only
        "-c"    # bytes only
        "-m"    # chars only
        "-lw"   # lines and words
        "-lc"   # lines and bytes
        "-wc"   # words and bytes
        "-lwc"  # all three
    )
    
    for file in "${files[@]}"; do
        for flags in "${flags_list[@]}"; do
            compare_outputs "$TEST_DIR/$file" "$flags" "$file with flags '$flags'"
        done
    done
}

# Test with large files
test_large_files() {
    log_info "Testing with large files..."
    
    compare_outputs "$TEST_DIR/large.txt" "" "large file default"
    compare_outputs "$TEST_DIR/large.txt" "-l" "large file lines"
    compare_outputs "$TEST_DIR/large.txt" "-w" "large file words"
    compare_outputs "$TEST_DIR/large.txt" "-c" "large file bytes"
    compare_outputs "$TEST_DIR/repeated.txt" "" "repeated pattern file"
    compare_outputs "$TEST_DIR/long_lines.txt" "" "long lines file"
}

# Test stdin functionality
test_stdin() {
    log_info "Testing stdin functionality..."
    
    # Test empty stdin
    local system_empty=$(echo -n "" | wc)
    local our_empty=$(echo -n "" | ./wc)
    if [[ "$system_empty" == "$our_empty" ]]; then
        log_success "Empty stdin test"
    else
        log_error "Empty stdin test failed"
        echo "  System: $system_empty"
        echo "  Ours:   $our_empty"
    fi
    
    # Test stdin with content
    local test_content="line1\nline2 word1 word2\nline3\n"
    local system_stdin=$(printf "$test_content" | wc)
    local our_stdin=$(printf "$test_content" | ./wc)
    if [[ "$system_stdin" == "$our_stdin" ]]; then
        log_success "Stdin with content test"
    else
        log_error "Stdin with content test failed"
        echo "  System: $system_stdin"
        echo "  Ours:   $our_stdin"
    fi
    
    # Test large stdin
    local system_large_stdin=$(seq 1 1000 | wc)
    local our_large_stdin=$(seq 1 1000 | ./wc)
    if [[ "$system_large_stdin" == "$our_large_stdin" ]]; then
        log_success "Large stdin test"
    else
        log_error "Large stdin test failed"
        echo "  System: $system_large_stdin"
        echo "  Ours:   $our_large_stdin"
    fi
}

# Test multiple files functionality
test_multiple_files() {
    log_info "Testing multiple files functionality..."
    
    # Create a few small test files
    echo "file1 content" > "$TEST_DIR/multi1.txt"
    printf "file2\nline2" > "$TEST_DIR/multi2.txt"
    printf "file3\nline2\nline3\n" > "$TEST_DIR/multi3.txt"
    
    # Test multiple files
    local system_multi=$(wc "$TEST_DIR/multi"*.txt 2>/dev/null)
    local our_multi=$(./wc "$TEST_DIR/multi"*.txt 2>/dev/null)
    
    if [[ "$system_multi" == "$our_multi" ]]; then
        log_success "Multiple files test"
    else
        log_error "Multiple files test failed"
        echo "System output:"
        echo "$system_multi"
        echo "Our output:"
        echo "$our_multi"
    fi
    
    # Clean up
    rm -f "$TEST_DIR/multi"*.txt
}

# Test error handling
test_error_handling() {
    log_info "Testing error handling..."
    
    # Test non-existent file
    local our_error=$(./wc /nonexistent/file 2>&1 >/dev/null || true)
    if [[ "$our_error" == *"No such file"* ]] || [[ "$our_error" == *"not found"* ]]; then
        log_success "Non-existent file error handling"
    else
        log_error "Non-existent file error handling failed"
        echo "  Error output: $our_error"
    fi
    
    # Test permission denied (try to create a file we can't read)
    if touch "$TEST_DIR/no_read.txt" 2>/dev/null && chmod 000 "$TEST_DIR/no_read.txt" 2>/dev/null; then
        local perm_error=$(./wc "$TEST_DIR/no_read.txt" 2>&1 >/dev/null || true)
        if [[ "$perm_error" == *"Permission denied"* ]]; then
            log_success "Permission denied error handling"
        else
            log_warning "Permission denied error handling unclear: $perm_error"
        fi
        chmod 644 "$TEST_DIR/no_read.txt" 2>/dev/null || true
        rm -f "$TEST_DIR/no_read.txt" 2>/dev/null || true
    fi
}

# Performance benchmarking
performance_benchmark() {
    log_info "Running performance benchmarks..."
    
    # Create a large test file for benchmarking
    log_info "Creating large benchmark file (50MB)..."
    seq 1 2000000 | sed 's/.*/This is line & with some additional text to make it realistic/' > "$TEST_DIR/benchmark.txt"
    
    log_info "Benchmarking system wc..."
    local system_time
    system_time=$(time (wc "$TEST_DIR/benchmark.txt" >/dev/null) 2>&1 | grep real | awk '{print $2}')
    
    log_info "Benchmarking our wc..."
    local our_time
    our_time=$(time (./wc "$TEST_DIR/benchmark.txt" >/dev/null) 2>&1 | grep real | awk '{print $2}')
    
    log_info "System wc time: $system_time"
    log_info "Our wc time: $our_time"
    
    # Verify outputs are identical
    local system_result=$(wc "$TEST_DIR/benchmark.txt")
    local our_result=$(./wc "$TEST_DIR/benchmark.txt")
    
    if [[ "$system_result" == "$our_result" ]]; then
        log_success "Performance benchmark - outputs match"
    else
        log_error "Performance benchmark - outputs differ"
        echo "  System: $system_result"
        echo "  Ours:   $our_result"
    fi
    
    # Clean up large file
    rm -f "$TEST_DIR/benchmark.txt"
}

# Memory usage test
test_memory_usage() {
    log_info "Testing memory usage..."
    
    # Create a very large file to test memory efficiency
    log_info "Creating very large test file (100MB)..."
    seq 1 4000000 | sed 's/.*/Line & with content/' > "$TEST_DIR/memory_test.txt"
    
    # Test if our implementation can handle it without excessive memory usage
    if ./wc "$TEST_DIR/memory_test.txt" >/dev/null 2>&1; then
        log_success "Large file memory test passed"
    else
        log_error "Large file memory test failed"
    fi
    
    # Clean up
    rm -f "$TEST_DIR/memory_test.txt"
}

# Corner cases and edge cases
test_corner_cases() {
    log_info "Testing corner cases..."
    
    # Test with binary data
    printf "\x00\x01\x02hello\nworld\x00\xff" > "$TEST_DIR/binary_test.txt"
    compare_outputs "$TEST_DIR/binary_test.txt" "" "binary data"
    
    # Test with very long single line
    python3 -c "print('x' * 100000, end='')" > "$TEST_DIR/very_long_line.txt"
    compare_outputs "$TEST_DIR/very_long_line.txt" "" "very long single line"
    
    # Test file with just one newline
    printf "\n" > "$TEST_DIR/just_newline.txt"
    compare_outputs "$TEST_DIR/just_newline.txt" "" "single newline file"
    
    # Test file with alternating pattern
    python3 -c "
for i in range(1000):
    print('a' if i % 2 == 0 else 'bb cc')
" > "$TEST_DIR/alternating.txt"
    compare_outputs "$TEST_DIR/alternating.txt" "" "alternating pattern"
    
    # Clean up corner case files
    rm -f "$TEST_DIR/binary_test.txt" "$TEST_DIR/very_long_line.txt" "$TEST_DIR/just_newline.txt" "$TEST_DIR/alternating.txt"
}

# Test command line options
test_command_line_options() {
    log_info "Testing command line options..."
    
    # Test help option
    if ./wc --help >/dev/null 2>&1; then
        log_success "Help option works"
    else
        log_error "Help option failed"
    fi
    
    # Test version option
    if ./wc --version >/dev/null 2>&1; then
        log_success "Version option works"
    else
        log_error "Version option failed"
    fi
    
    # Test invalid option
    if ! ./wc --invalid-option >/dev/null 2>&1; then
        log_success "Invalid option handling works"
    else
        log_error "Invalid option should fail"
    fi
}

# Stress testing
stress_test() {
    log_info "Running stress tests..."
    
    # Test with many files
    log_info "Creating many small files..."
    for i in {1..100}; do
        echo "Content of file $i" > "$TEST_DIR/stress_$i.txt"
    done
    
    # Test our wc with many files
    if ./wc "$TEST_DIR/stress_"*.txt >/dev/null 2>&1; then
        log_success "Many files stress test passed"
    else
        log_error "Many files stress test failed"
    fi
    
    # Clean up stress test files
    rm -f "$TEST_DIR/stress_"*.txt
    
    # Test with pathological input (many consecutive newlines)
    python3 -c "print('\\n' * 100000, end='')" > "$TEST_DIR/many_newlines.txt"
    compare_outputs "$TEST_DIR/many_newlines.txt" "" "many consecutive newlines"
    
    # Test with pathological input (very long words)
    python3 -c "print(' '.join(['x' * 1000 for _ in range(100)]))" > "$TEST_DIR/long_words.txt"
    compare_outputs "$TEST_DIR/long_words.txt" "" "very long words"
    
    # Clean up
    rm -f "$TEST_DIR/many_newlines.txt" "$TEST_DIR/long_words.txt"
}

# Clean up test files
cleanup() {
    log_info "Cleaning up test files..."
    rm -rf "$TEST_DIR"
    log_success "Cleanup completed"
}

# Print summary
print_summary() {
    echo
    echo "==============================================="
    echo "TEST SUMMARY"
    echo "==============================================="
    echo -e "Tests passed: ${GREEN}$TESTS_PASSED${NC}"
    echo -e "Tests failed: ${RED}$TESTS_FAILED${NC}"
    echo -e "Total tests:  $(($TESTS_PASSED + $TESTS_FAILED))"
    echo
    
    if [[ $TESTS_FAILED -eq 0 ]]; then
        echo -e "${GREEN}ALL TESTS PASSED!${NC} 🎉"
        echo "The wc implementation appears to be working correctly."
    else
        echo -e "${RED}SOME TESTS FAILED!${NC} 😞"
        echo "Please check the failed tests above."
        exit 1
    fi
}

# Main test execution
main() {
    echo "==============================================="
    echo "COMPREHENSIVE WC UTILITY TEST SUITE"
    echo "==============================================="
    echo
    
    check_prerequisites
    create_test_files
    
    test_basic_functionality
    test_large_files
    test_stdin
    test_multiple_files
    test_error_handling
    test_corner_cases
    test_command_line_options
    stress_test
    performance_benchmark
    test_memory_usage
    
    cleanup
    print_summary
}

# Run main function
main "$@"
