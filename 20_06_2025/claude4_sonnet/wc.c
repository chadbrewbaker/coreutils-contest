#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <getopt.h>
#include <ctype.h>
#include <assert.h>
#include <time.h>

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

// Structure to hold counts
typedef struct {
    size_t lines;
    size_t words;
    size_t chars;
    size_t bytes;
} wc_counts_t;

// Options structure
typedef struct {
    int count_lines;
    int count_words;
    int count_chars;
    int count_bytes;
    int max_line_length;
} wc_options_t;

// SIMD-optimized line counting for ARM64
static size_t count_lines_simd(const char *data, size_t size) {
#ifdef __ARM_NEON
    const char *ptr = data;
    const char *end = data + size;
    size_t count = 0;
    
    // Process 16 bytes at a time with NEON
    const char *simd_end = end - (end - ptr) % 16;
    uint8x16_t newline_vec = vdupq_n_u8('\n');
    
    while (ptr < simd_end) {
        uint8x16_t chunk = vld1q_u8((const uint8_t*)ptr);
        uint8x16_t cmp = vceqq_u8(chunk, newline_vec);
        
        // Count set bits in comparison result
        uint64x2_t pairs = vreinterpretq_u64_u8(cmp);
        uint64_t low = vgetq_lane_u64(pairs, 0);
        uint64_t high = vgetq_lane_u64(pairs, 1);
        
        count += __builtin_popcountll(low) / 8;
        count += __builtin_popcountll(high) / 8;
        
        ptr += 16;
    }
    
    // Handle remaining bytes
    while (ptr < end) {
        if (*ptr == '\n') count++;
        ptr++;
    }
    
    return count;
#else
    // Fallback for non-NEON systems
    size_t count = 0;
    for (size_t i = 0; i < size; i++) {
        if (data[i] == '\n') count++;
    }
    return count;
#endif
}

// Optimized word counting with state machine
static size_t count_words_optimized(const char *data, size_t size) {
    if (size == 0) return 0;
    
    size_t count = 0;
    int in_word = 0;
    
    for (size_t i = 0; i < size; i++) {
        char c = data[i];
        int is_space = (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v');
        
        if (!is_space && !in_word) {
            count++;
            in_word = 1;
        } else if (is_space) {
            in_word = 0;
        }
    }
    
    return count;
}

// Fast character counting (just return size for ASCII)
static size_t count_chars_ascii(const char *data, size_t size) {
    return size;
}

// Main counting function
static wc_counts_t count_data(const char *data, size_t size, const wc_options_t *opts) {
    wc_counts_t counts = {0};
    
    if (opts->count_bytes) {
        counts.bytes = size;
    }
    
    if (opts->count_chars) {
        counts.chars = count_chars_ascii(data, size);
    }
    
    if (opts->count_lines) {
        counts.lines = count_lines_simd(data, size);
    }
    
    if (opts->count_words) {
        counts.words = count_words_optimized(data, size);
    }
    
    return counts;
}

// Process file using memory mapping for large files
static wc_counts_t process_file(const char *filename, const wc_options_t *opts) {
    wc_counts_t counts = {0};
    
    int fd = (filename && strcmp(filename, "-") != 0) ? 
             open(filename, O_RDONLY) : STDIN_FILENO;
    
    if (fd == -1) {
        fprintf(stderr, "wc: %s: %s\n", filename ? filename : "stdin", strerror(errno));
        return counts;
    }
    
    struct stat st;
    if (fstat(fd, &st) == -1) {
        fprintf(stderr, "wc: %s: %s\n", filename ? filename : "stdin", strerror(errno));
        if (fd != STDIN_FILENO) close(fd);
        return counts;
    }
    
    // Use mmap for regular files larger than 4KB
    if (S_ISREG(st.st_mode) && st.st_size > 4096) {
        void *data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (data != MAP_FAILED) {
            counts = count_data((const char*)data, st.st_size, opts);
            munmap(data, st.st_size);
        } else {
            fprintf(stderr, "wc: %s: mmap failed: %s\n", filename ? filename : "stdin", strerror(errno));
        }
    } else {
        // Read in chunks for stdin or small files
        const size_t BUFFER_SIZE = 64 * 1024;
        char *buffer = malloc(BUFFER_SIZE);
        if (!buffer) {
            fprintf(stderr, "wc: memory allocation failed\n");
            if (fd != STDIN_FILENO) close(fd);
            return counts;
        }
        
        size_t total_size = 0;
        char *all_data = NULL;
        
        ssize_t bytes_read;
        while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
            all_data = realloc(all_data, total_size + bytes_read);
            if (!all_data) {
                fprintf(stderr, "wc: memory allocation failed\n");
                free(buffer);
                if (fd != STDIN_FILENO) close(fd);
                return counts;
            }
            memcpy(all_data + total_size, buffer, bytes_read);
            total_size += bytes_read;
        }
        
        if (all_data) {
            counts = count_data(all_data, total_size, opts);
            free(all_data);
        }
        
        free(buffer);
    }
    
    if (fd != STDIN_FILENO) close(fd);
    return counts;
}

// Print results
static void print_counts(const wc_counts_t *counts, const wc_options_t *opts, const char *filename) {
    if (opts->count_lines) printf("%8zu ", counts->lines);
    if (opts->count_words) printf("%8zu ", counts->words);
    if (opts->count_chars) printf("%8zu ", counts->chars);
    if (opts->count_bytes && !opts->count_chars) printf("%8zu ", counts->bytes);
    
    if (filename && strcmp(filename, "-") != 0) {
        printf("%s", filename);
    }
    printf("\n");
}

// Usage information
static void usage(void) {
    printf("Usage: wc [OPTION]... [FILE]...\n");
    printf("Print newline, word, and byte counts for each FILE.\n\n");
    printf("  -c, --bytes            print the byte counts\n");
    printf("  -m, --chars            print the character counts\n");
    printf("  -l, --lines            print the newline counts\n");
    printf("  -L, --max-line-length  print the maximum display width\n");
    printf("  -w, --words            print the word counts\n");
    printf("      --help             display this help and exit\n");
    printf("      --version          output version information and exit\n");
}

int main(int argc, char *argv[]) {
    wc_options_t opts = {0};
    int opt;
    
    static struct option long_options[] = {
        {"bytes", no_argument, 0, 'c'},
        {"chars", no_argument, 0, 'm'},
        {"lines", no_argument, 0, 'l'},
        {"max-line-length", no_argument, 0, 'L'},
        {"words", no_argument, 0, 'w'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };
    
    while ((opt = getopt_long(argc, argv, "cmlLwh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'c': opts.count_bytes = 1; break;
            case 'm': opts.count_chars = 1; break;
            case 'l': opts.count_lines = 1; break;
            case 'L': opts.max_line_length = 1; break;
            case 'w': opts.count_words = 1; break;
            case 'h': usage(); return 0;
            case 'v': printf("wc (efficient) 1.0\n"); return 0;
            default: usage(); return 1;
        }
    }
    
    // Default behavior: count lines, words, and bytes
    if (!opts.count_lines && !opts.count_words && !opts.count_chars && 
        !opts.count_bytes && !opts.max_line_length) {
        opts.count_lines = opts.count_words = opts.count_bytes = 1;
    }
    
    wc_counts_t total_counts = {0};
    int file_count = 0;
    
    if (optind >= argc) {
        // No files specified, read from stdin
        wc_counts_t counts = process_file("-", &opts);
        print_counts(&counts, &opts, NULL);
    } else {
        // Process each file
        for (int i = optind; i < argc; i++) {
            wc_counts_t counts = process_file(argv[i], &opts);
            print_counts(&counts, &opts, argv[i]);
            
            total_counts.lines += counts.lines;
            total_counts.words += counts.words;
            total_counts.chars += counts.chars;
            total_counts.bytes += counts.bytes;
            file_count++;
        }
        
        // Print total if multiple files
        if (file_count > 1) {
            print_counts(&total_counts, &opts, "total");
        }
    }
    
    return 0;
}

// ============================================================================
// UNIT TESTS
// ============================================================================

#ifdef UNIT_TESTS
#include <assert.h>

void test_count_lines_simd() {
    printf("Testing count_lines_simd...\n");
    
    // Test empty string
    assert(count_lines_simd("", 0) == 0);
    
    // Test single line without newline
    assert(count_lines_simd("hello", 5) == 0);
    
    // Test single line with newline
    assert(count_lines_simd("hello\n", 6) == 1);
    
    // Test multiple lines
    assert(count_lines_simd("line1\nline2\nline3\n", 18) == 3);
    
    // Test with various line endings
    assert(count_lines_simd("a\nb\nc\n", 6) == 3);
    
    // Test large string (over SIMD chunk size)
    char large[100];
    memset(large, 'a', 99);
    large[99] = '\0';
    for (int i = 10; i < 99; i += 10) {
        large[i] = '\n';
    }
    size_t expected_lines = 8; // newlines at positions 10,20,30,40,50,60,70,80
    assert(count_lines_simd(large, 99) == expected_lines);
    
    printf("✓ count_lines_simd tests passed\n");
}

void test_count_words_optimized() {
    printf("Testing count_words_optimized...\n");
    
    // Test empty string
    assert(count_words_optimized("", 0) == 0);
    
    // Test single word
    assert(count_words_optimized("hello", 5) == 1);
    
    // Test multiple words
    assert(count_words_optimized("hello world", 11) == 2);
    
    // Test words with various whitespace
    assert(count_words_optimized("  hello   world  ", 17) == 2);
    
    // Test words with tabs and newlines
    assert(count_words_optimized("hello\tworld\ntest", 16) == 3);
    
    // Test only whitespace
    assert(count_words_optimized("   \t\n  ", 7) == 0);
    
    printf("✓ count_words_optimized tests passed\n");
}

void test_count_chars_ascii() {
    printf("Testing count_chars_ascii...\n");
    
    // Test empty string
    assert(count_chars_ascii("", 0) == 0);
    
    // Test regular string
    assert(count_chars_ascii("hello", 5) == 5);
    
    // Test string with special characters
    assert(count_chars_ascii("hello\n\tworld", 12) == 12);
    
    printf("✓ count_chars_ascii tests passed\n");
}

void test_count_data() {
    printf("Testing count_data...\n");
    
    wc_options_t opts = {1, 1, 1, 1, 0}; // Count all
    
    // Test empty data
    wc_counts_t counts = count_data("", 0, &opts);
    assert(counts.lines == 0);
    assert(counts.words == 0);
    assert(counts.chars == 0);
    assert(counts.bytes == 0);
    
    // Test simple string
    const char *test_str = "hello world\ntest line\n";
    counts = count_data(test_str, strlen(test_str), &opts);
    assert(counts.lines == 2);
    assert(counts.words == 4);
    assert(counts.chars == strlen(test_str));
    assert(counts.bytes == strlen(test_str));
    
    printf("✓ count_data tests passed\n");
}

void run_unit_tests() {
    printf("Running unit tests...\n");
    test_count_lines_simd();
    test_count_words_optimized();
    test_count_chars_ascii();
    test_count_data();
    printf("All unit tests passed!\n\n");
}
#endif

// ============================================================================
// INTEGRATION TESTS
// ============================================================================

#ifdef INTEGRATION_TESTS
#include <sys/wait.h>

void create_test_file(const char *filename, const char *content) {
    FILE *f = fopen(filename, "w");
    if (f) {
        fputs(content, f);
        fclose(f);
    }
}

void test_integration() {
    printf("Running integration tests...\n");
    
    // Test 1: Empty file
    create_test_file("test_empty.txt", "");
    system("./wc test_empty.txt > test_output.txt");
    
    // Test 2: Single line file
    create_test_file("test_single.txt", "hello world");
    system("./wc test_single.txt > test_output.txt");
    
    // Test 3: Multi-line file
    create_test_file("test_multi.txt", "line one\nline two\nline three\n");
    system("./wc test_multi.txt > test_output.txt");
    
    // Test 4: File with only whitespace
    create_test_file("test_whitespace.txt", "   \t\n  \n\n");
    system("./wc test_whitespace.txt > test_output.txt");
    
    // Test 5: Large file
    FILE *large_file = fopen("test_large.txt", "w");
    if (large_file) {
        for (int i = 0; i < 10000; i++) {
            fprintf(large_file, "This is line %d with some words\n", i);
        }
        fclose(large_file);
    }
    system("./wc test_large.txt > test_output.txt");
    
    // Test 6: Binary-like content
    create_test_file("test_binary.txt", "hello\0world\ntest\0\0line\n");
    system("./wc test_binary.txt > test_output.txt");
    
    // Clean up
    system("rm -f test_*.txt");
    
    printf("✓ Integration tests completed\n");
}
#endif

// ============================================================================
// PERFORMANCE TESTS
// ============================================================================

#ifdef PERFORMANCE_TESTS
double get_time_diff(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

void performance_test() {
    printf("Running performance tests...\n");
    
    // Generate large test data
    const size_t test_size = 10 * 1024 * 1024; // 10MB
    char *test_data = malloc(test_size);
    if (!test_data) {
        printf("Failed to allocate memory for performance test\n");
        return;
    }
    
    // Fill with realistic text data
    size_t pos = 0;
    while (pos < test_size - 100) {
        int line_len = 50 + (rand() % 50);
        for (int i = 0; i < line_len && pos < test_size - 1; i++) {
            if (i > 0 && (rand() % 8) == 0) {
                test_data[pos++] = ' ';
            } else {
                test_data[pos++] = 'a' + (rand() % 26);
            }
        }
        if (pos < test_size - 1) {
            test_data[pos++] = '\n';
        }
    }
    test_data[test_size - 1] = '\0';
    
    wc_options_t opts = {1, 1, 1, 1, 0};
    struct timespec start, end;
    const int iterations = 100;
    
    // Test SIMD line counting
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < iterations; i++) {
        volatile size_t lines = count_lines_simd(test_data, test_size);
        (void)lines;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    double simd_time = get_time_diff(start, end);
    
    // Test word counting
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < iterations; i++) {
        volatile size_t words = count_words_optimized(test_data, test_size);
        (void)words;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    double word_time = get_time_diff(start, end);
    
    // Test full counting
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < iterations; i++) {
        wc_counts_t counts = count_data(test_data, test_size, &opts);
        (void)counts;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    double full_time = get_time_diff(start, end);
    
    printf("Performance results (%d iterations on %.1fMB):\n", iterations, test_size / 1024.0 / 1024.0);
    printf("  SIMD line counting: %.3f seconds (%.1f MB/s)\n", 
           simd_time, (test_size * iterations) / (simd_time * 1024 * 1024));
    printf("  Word counting: %.3f seconds (%.1f MB/s)\n", 
           word_time, (test_size * iterations) / (word_time * 1024 * 1024));
    printf("  Full counting: %.3f seconds (%.1f MB/s)\n", 
           full_time, (test_size * iterations) / (full_time * 1024 * 1024));
    
    free(test_data);
}
#endif

// ============================================================================
// STRESS TESTS
// ============================================================================

#ifdef STRESS_TESTS
void stress_test() {
    printf("Running stress tests...\n");
    
    // Test with very large lines
    const size_t huge_line_size = 1024 * 1024; // 1MB line
    char *huge_line = malloc(huge_line_size + 10);
    if (huge_line) {
        memset(huge_line, 'a', huge_line_size);
        huge_line[huge_line_size] = '\n';
        huge_line[huge_line_size + 1] = '\0';
        
        wc_options_t opts = {1, 1, 1, 1, 0};
        wc_counts_t counts = count_data(huge_line, huge_line_size + 1, &opts);
        
        printf("Huge line test: %zu lines, %zu words, %zu chars\n", 
               counts.lines, counts.words, counts.chars);
        
        free(huge_line);
    }
    
    // Test with many small lines
    const size_t many_lines_size = 1024 * 1024;
    char *many_lines = malloc(many_lines_size);
    if (many_lines) {
        size_t pos = 0;
        while (pos < many_lines_size - 2) {
            many_lines[pos++] = 'x';
            many_lines[pos++] = '\n';
        }
        many_lines[pos] = '\0';
        
        wc_options_t opts = {1, 1, 1, 1, 0};
        wc_counts_t counts = count_data(many_lines, pos, &opts);
        
        printf("Many lines test: %zu lines, %zu words, %zu chars\n", 
               counts.lines, counts.words, counts.chars);
        
        free(many_lines);
    }
    
    printf("✓ Stress tests completed\n");
}
#endif
