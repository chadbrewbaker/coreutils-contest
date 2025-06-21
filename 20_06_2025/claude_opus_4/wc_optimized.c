// wc_optimized.c - Efficient wc implementation for ASCII strings on Mac M1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <arm_neon.h>
#include <time.h>
#include <assert.h>
#include <errno.h>

#define BUFFER_SIZE (1024 * 1024)  // 1MB buffer for non-mmap reads
#define MIN_MMAP_SIZE (4096)       // Minimum file size for mmap

typedef struct {
    size_t lines;
    size_t words;
    size_t bytes;
} counts_t;

// SIMD-optimized newline counter using NEON
static inline size_t count_newlines_neon(const uint8_t *data, size_t len) {
    size_t count = 0;
    const uint8_t newline = '\n';
    uint8x16_t nl_vec = vdupq_n_u8(newline);
    
    // Process 64 bytes at a time (4 x 16 bytes)
    while (len >= 64) {
        uint8x16_t v0 = vld1q_u8(data);
        uint8x16_t v1 = vld1q_u8(data + 16);
        uint8x16_t v2 = vld1q_u8(data + 32);
        uint8x16_t v3 = vld1q_u8(data + 48);
        
        uint8x16_t eq0 = vceqq_u8(v0, nl_vec);
        uint8x16_t eq1 = vceqq_u8(v1, nl_vec);
        uint8x16_t eq2 = vceqq_u8(v2, nl_vec);
        uint8x16_t eq3 = vceqq_u8(v3, nl_vec);
        
        // Count set bits (matches)
        count += vaddvq_u8(eq0) / 255;
        count += vaddvq_u8(eq1) / 255;
        count += vaddvq_u8(eq2) / 255;
        count += vaddvq_u8(eq3) / 255;
        
        data += 64;
        len -= 64;
    }
    
    // Process 16 bytes at a time
    while (len >= 16) {
        uint8x16_t v = vld1q_u8(data);
        uint8x16_t eq = vceqq_u8(v, nl_vec);
        count += vaddvq_u8(eq) / 255;
        data += 16;
        len -= 16;
    }
    
    // Handle remaining bytes
    while (len--) {
        if (*data++ == '\n') count++;
    }
    
    return count;
}

// Optimized word counting with state machine
static void count_words_and_lines(const uint8_t *data, size_t len, counts_t *c) {
    int in_word = 0;
    size_t i = 0;
    
    // Unroll loop for better performance
    while (i + 8 <= len) {
        for (int j = 0; j < 8; j++) {
            uint8_t ch = data[i + j];
            if (ch == '\n') c->lines++;
            
            int is_space = (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r');
            if (in_word && is_space) {
                c->words++;
                in_word = 0;
            } else if (!in_word && !is_space) {
                in_word = 1;
            }
        }
        i += 8;
    }
    
    // Handle remaining bytes
    while (i < len) {
        uint8_t ch = data[i++];
        if (ch == '\n') c->lines++;
        
        int is_space = (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r');
        if (in_word && is_space) {
            c->words++;
            in_word = 0;
        } else if (!in_word && !is_space) {
            in_word = 1;
        }
    }
    
    // Count last word if needed
    if (in_word) c->words++;
}

// Process file using mmap for large files
static int process_file_mmap(const char *filename, counts_t *c) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) return -1;
    
    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        return -1;
    }
    
    if (st.st_size == 0) {
        close(fd);
        return 0;
    }
    
    void *map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    
    if (map == MAP_FAILED) return -1;
    
    // Advise kernel about access pattern
    madvise(map, st.st_size, MADV_SEQUENTIAL);
    
    c->bytes = st.st_size;
    count_words_and_lines((const uint8_t *)map, st.st_size, c);
    
    munmap(map, st.st_size);
    return 0;
}

// Process file using buffered reads for small files or stdin
static int process_file_buffered(FILE *fp, counts_t *c) {
    uint8_t *buffer = aligned_alloc(64, BUFFER_SIZE);
    if (!buffer) return -1;
    
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
        c->bytes += bytes_read;
        count_words_and_lines(buffer, bytes_read, c);
    }
    
    free(buffer);
    return ferror(fp) ? -1 : 0;
}

// Main wc function
static int wc(const char *filename, counts_t *c) {
    memset(c, 0, sizeof(counts_t));
    
    if (!filename || strcmp(filename, "-") == 0) {
        // Read from stdin
        return process_file_buffered(stdin, c);
    }
    
    struct stat st;
    if (stat(filename, &st) < 0) return -1;
    
    // Use mmap for regular files larger than MIN_MMAP_SIZE
    if (S_ISREG(st.st_mode) && st.st_size >= MIN_MMAP_SIZE) {
        return process_file_mmap(filename, c);
    }
    
    // Use buffered I/O for small files or non-regular files
    FILE *fp = fopen(filename, "r");
    if (!fp) return -1;
    
    int ret = process_file_buffered(fp, c);
    fclose(fp);
    return ret;
}

// ============= UNIT TESTS =============
#ifdef RUN_TESTS

static void test_newline_counter() {
    printf("Testing NEON newline counter...\n");
    
    // Test empty
    assert(count_newlines_neon((uint8_t*)"", 0) == 0);
    
    // Test single newline
    assert(count_newlines_neon((uint8_t*)"\n", 1) == 1);
    
    // Test multiple newlines
    assert(count_newlines_neon((uint8_t*)"\n\n\n", 3) == 3);
    
    // Test mixed content
    const char *mixed = "Hello\nWorld\n\nTest\n";
    assert(count_newlines_neon((uint8_t*)mixed, strlen(mixed)) == 4);
    
    // Test large buffer (>64 bytes)
    char large[200];
    memset(large, 'a', sizeof(large));
    for (int i = 10; i < 200; i += 20) large[i] = '\n';
    assert(count_newlines_neon((uint8_t*)large, sizeof(large)) == 9);
    
    printf("✓ Newline counter tests passed\n");
}

static void test_word_counting() {
    printf("Testing word counting...\n");
    
    counts_t c;
    
    // Test empty
    memset(&c, 0, sizeof(c));
    count_words_and_lines((uint8_t*)"", 0, &c);
    assert(c.words == 0 && c.lines == 0);
    
    // Test single word
    memset(&c, 0, sizeof(c));
    count_words_and_lines((uint8_t*)"hello", 5, &c);
    assert(c.words == 1 && c.lines == 0);
    
    // Test multiple words
    memset(&c, 0, sizeof(c));
    count_words_and_lines((uint8_t*)"hello world test", 16, &c);
    assert(c.words == 3 && c.lines == 0);
    
    // Test with newlines
    memset(&c, 0, sizeof(c));
    count_words_and_lines((uint8_t*)"hello\nworld\n", 12, &c);
    assert(c.words == 2 && c.lines == 2);
    
    // Test multiple spaces
    memset(&c, 0, sizeof(c));
    count_words_and_lines((uint8_t*)"hello   world", 13, &c);
    assert(c.words == 2);
    
    // Test tabs
    memset(&c, 0, sizeof(c));
    count_words_and_lines((uint8_t*)"hello\tworld\ttesting", 19, &c);
    assert(c.words == 3);
    
    printf("✓ Word counting tests passed\n");
}

static void create_test_file(const char *filename, const char *content) {
    FILE *fp = fopen(filename, "w");
    assert(fp != NULL);
    if (content) fputs(content, fp);
    fclose(fp);
}

static void test_integration() {
    printf("Testing integration...\n");
    
    counts_t c;
    
    // Test empty file
    create_test_file("test_empty.txt", "");
    assert(wc("test_empty.txt", &c) == 0);
    assert(c.lines == 0 && c.words == 0 && c.bytes == 0);
    unlink("test_empty.txt");
    
    // Test simple file
    create_test_file("test_simple.txt", "Hello world\n");
    assert(wc("test_simple.txt", &c) == 0);
    assert(c.lines == 1 && c.words == 2 && c.bytes == 12);
    unlink("test_simple.txt");
    
    // Test file without final newline
    create_test_file("test_no_nl.txt", "Hello world");
    assert(wc("test_no_nl.txt", &c) == 0);
    assert(c.lines == 0 && c.words == 2 && c.bytes == 11);
    unlink("test_no_nl.txt");
    
    // Test multi-line file
    create_test_file("test_multi.txt", "Line 1\nLine 2\nLine 3\n");
    assert(wc("test_multi.txt", &c) == 0);
    assert(c.lines == 3 && c.words == 6 && c.bytes == 21);
    unlink("test_multi.txt");
    
    // Test non-existent file
    assert(wc("non_existent_file.txt", &c) == -1);
    
    printf("✓ Integration tests passed\n");
}

static void run_performance_test() {
    printf("\nPerformance Tests:\n");
    
    // Create test files of various sizes
    const size_t sizes[] = {1024, 1024*1024, 10*1024*1024};
    const char *size_names[] = {"1KB", "1MB", "10MB"};
    
    for (int i = 0; i < 3; i++) {
        char filename[32];
        snprintf(filename, sizeof(filename), "perf_test_%zu.txt", sizes[i]);
        
        // Create file with realistic content
        FILE *fp = fopen(filename, "w");
        assert(fp != NULL);
        
        for (size_t j = 0; j < sizes[i]; j += 80) {
            fprintf(fp, "The quick brown fox jumps over the lazy dog. Testing performance here.\n");
        }
        fclose(fp);
        
        // Time the operation
        counts_t c;
        clock_t start = clock();
        assert(wc(filename, &c) == 0);
        clock_t end = clock();
        
        double cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
        double throughput = (sizes[i] / (1024.0 * 1024.0)) / cpu_time;
        
        printf("  %s file: %.3f ms, %.1f MB/s (lines: %zu, words: %zu)\n",
               size_names[i], cpu_time * 1000, throughput, c.lines, c.words);
        
        unlink(filename);
    }
}

#endif // RUN_TESTS

// ============= MAIN PROGRAM =============
int main(int argc, char *argv[]) {
#ifdef RUN_TESTS
    printf("Running tests...\n\n");
    test_newline_counter();
    test_word_counting();
    test_integration();
    run_performance_test();
    printf("\nAll tests passed!\n");
    return 0;
#else
    counts_t total = {0, 0, 0};
    int file_count = 0;
    int exit_code = 0;
    
    if (argc == 1) {
        // Read from stdin
        if (wc(NULL, &total) < 0) {
            perror("wc");
            return 1;
        }
        printf("%8zu %8zu %8zu\n", total.lines, total.words, total.bytes);
    } else {
        // Process files
        for (int i = 1; i < argc; i++) {
            counts_t c;
            if (wc(argv[i], &c) < 0) {
                fprintf(stderr, "wc: %s: %s\n", argv[i], strerror(errno));
                exit_code = 1;
                continue;
            }
            
            printf("%8zu %8zu %8zu %s\n", c.lines, c.words, c.bytes, argv[i]);
            
            total.lines += c.lines;
            total.words += c.words;
            total.bytes += c.bytes;
            file_count++;
        }
        
        // Print total if multiple files
        if (file_count > 1) {
            printf("%8zu %8zu %8zu total\n", total.lines, total.words, total.bytes);
        }
    }
    
    return exit_code;
#endif
}

// Compilation instructions:
// For normal use: clang -O3 -march=native wc_optimized.c -o wc_optimized
// For testing: clang -O3 -march=native -DRUN_TESTS wc_optimized.c -o wc_test