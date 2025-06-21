#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>

// Structure to hold counts
typedef struct {
    long lines;
    long words;
    long chars;
} Counts;

// Function to process a memory-mapped buffer
Counts process_buffer(const char *buffer, size_t size) {
    Counts counts = {0, 0, 0};
    int in_word = 0;

    for (size_t i = 0; i < size; i++) {
        counts.chars++;
        if (buffer[i] == '\n') {
            counts.lines++;
        }
        if (isspace(buffer[i])) {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            counts.words++;
        }
    }
    return counts;
}

// Function to process a file
Counts process_file(const char *filename) {
    Counts counts = {0, 0, 0};
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "wc: %s: No such file or directory\n", filename);
        return counts;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        return counts;
    }

    if (st.st_size == 0) {
        close(fd);
        return counts;
    }

    // Memory map the file for efficiency
    char *buffer = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (buffer == MAP_FAILED) {
        close(fd);
        return counts;
    }

    counts = process_buffer(buffer, st.st_size);
    munmap(buffer, st.st_size);
    close(fd);
    return counts;
}

// Function to process standard input
Counts process_stdin(void) {
    Counts counts = {0, 0, 0};
    char buffer[8192]; // 8KB buffer for efficiency
    size_t bytes_read;
    int in_word = 0;

    while ((bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
        for (size_t i = 0; i < bytes_read; i++) {
            counts.chars++;
            if (buffer[i] == '\n') {
                counts.lines++;
            }
            if (isspace(buffer[i])) {
                in_word = 0;
            } else if (!in_word) {
                in_word = 1;
                counts.words++;
            }
        }
    }
    return counts;
}

// Unit test framework
void assert_equal(long expected, long actual, const char *msg) {
    if (expected != actual) {
        fprintf(stderr, "Test failed: %s (expected %ld, got %ld)\n", msg, expected, actual);
        exit(1);
    }
}

// Unit tests
void run_unit_tests(void) {
    printf("Running unit tests...\n");

    // Test 1: Empty string
    Counts c1 = process_buffer("", 0);
    assert_equal(0, c1.lines, "Empty string lines");
    assert_equal(0, c1.words, "Empty string words");
    assert_equal(0, c1.chars, "Empty string chars");

    // Test 2: Single line with one word
    Counts c2 = process_buffer("hello\n", 6);
    assert_equal(1, c2.lines, "Single line lines");
    assert_equal(1, c2.words, "Single line words");
    assert_equal(6, c2.chars, "Single line chars");

    // Test 3: Multiple words, no newline
    Counts c3 = process_buffer("hello world", 11);
    assert_equal(0, c3.lines, "No newline lines");
    assert_equal(2, c3.words, "No newline words");
    assert_equal(11, c3.chars, "No newline chars");

    // Test 4: Multiple lines with spaces
    Counts c4 = process_buffer("hello world\n  test  \n", 21);
    assert_equal(2, c4.lines, "Multiple lines lines");
    assert_equal(3, c4.words, "Multiple lines words");
    assert_equal(21, c4.chars, "Multiple lines chars");

    // Test 5: Only newlines
    Counts c5 = process_buffer("\n\n\n", 3);
    assert_equal(3, c5.lines, "Only newlines lines");
    assert_equal(0, c5.words, "Only newlines words");
    assert_equal(3, c5.chars, "Only newlines chars");

    printf("All unit tests passed!\n");
}

// Integration test helper: Create a temporary file
void create_temp_file(const char *filename, const char *content) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("Failed to create temp file");
        exit(1);
    }
    fwrite(content, 1, strlen(content), f);
    fclose(f);
}

// Integration tests
void run_integration_tests(void) {
    printf("Running integration tests...\n");

    // Test 1: Empty file
    create_temp_file("test_empty.txt", "");
    Counts c1 = process_file("test_empty.txt");
    assert_equal(0, c1.lines, "Empty file lines");
    assert_equal(0, c1.words, "Empty file words");
    assert_equal(0, c1.chars, "Empty file chars");
    unlink("test_empty.txt");

    // Test 2: File with one word
    create_temp_file("test_one.txt", "hello");
    Counts c2 = process_file("test_one.txt");
    assert_equal(0, c2.lines, "One word file lines");
    assert_equal(1, c2.words, "One word file words");
    assert_equal(5, c2.chars, "One word file chars");
    unlink("test_one.txt");

    // Test 3: Large file (1MB of repeated text)
    char *large_content = malloc(1024 * 1024 + 1);
    for (int i = 0; i < 1024 * 1024; i += 6) {
        strcpy(large_content + i, "hello ");
    }
    create_temp_file("test_large.txt", large_content);
    Counts c3 = process_file("test_large.txt");
    assert_equal(0, c3.lines, "Large file lines");
    assert_equal(1024 * 1024 / 6, c3.words, "Large file words");
    assert_equal(1024 * 1024, c3.chars, "Large file chars");
    unlink("test_large.txt");
    free(large_content);

    printf("All integration tests passed!\n");
}

// Performance test
void run_performance_test(void) {
    printf("Running performance test...\n");
    char *large_content = malloc(10 * 1024 * 1024 + 1); // 10MB
    for (size_t i = 0; i < 10 * 1024 * 1024; i += 6) {
        strcpy(large_content + i, "hello ");
    }
    create_temp_file("test_perf.txt", large_content);

    clock_t start = clock();
    Counts c = process_file("test_perf.txt");
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Performance test: Processed 10MB file\n");
    printf("Lines: %ld, Words: %ld, Chars: %ld\n", c.lines, c.words, c.chars);
    printf("Time: %.3f seconds\n", time_spent);

    unlink("test_perf.txt");
    free(large_content);
}

int main(int argc, char *argv[]) {
    int show_lines = 0, show_words = 0, show_chars = 0;
    int opt;

    // If no options specified, default to all counts
    if (argc == 1 || (argc == 2 && argv[1][0] != '-')) {
        show_lines = show_words = show_chars = 1;
    }

    // Parse options
    while ((opt = getopt(argc, argv, "lwc")) != -1) {
        switch (opt) {
            case 'l': show_lines = 1; break;
            case 'w': show_words = 1; break;
            case 'c': show_chars = 1; break;
            default:
                fprintf(stderr, "Usage: %s [-lwc] [file...]\n", argv[0]);
                return 1;
        }
    }

    // Run tests if specified
    if (argc >= 2 && strcmp(argv[1], "--test") == 0) {
        run_unit_tests();
        run_integration_tests();
        run_performance_test();
        return 0;
    }

    Counts total = {0, 0, 0};
    int file_count = 0;

    if (optind == argc) {
        // Process stdin
        Counts c = process_stdin();
        total.lines += c.lines;
        total.words += c.words;
        total.chars += c.chars;
        printf("%8ld %8ld %8ld\n", 
               show_lines ? c.lines : 0,
               show_words ? c.words : 0,
               show_chars ? c.chars : 0);
    } else {
        // Process files
        for (int i = optind; i < argc; i++) {
            Counts c = process_file(argv[i]);
            total.lines += c.lines;
            total.words += c.words;
            total.chars += c.chars;
            file_count++;
            if (c.chars > 0 || strcmp(argv[i], "-") != 0) {
                printf("%8ld %8ld %8ld %s\n", 
                       show_lines ? c.lines : 0,
                       show_words ? c.words : 0,
                       show_chars ? c.chars : 0,
                       argv[i]);
            }
        }
    }

    // Print total if multiple files
    if (file_count > 1) {
        printf("%8ld %8ld %8ld total\n", 
               show_lines ? total.lines : 0,
               show_words ? total.words : 0,
               show_chars ? total.chars : 0);
    }

    return 0;
}
