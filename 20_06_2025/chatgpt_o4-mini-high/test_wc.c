// test_wc.c
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include "wc.c"  // bring in count_buffer & process_fd

// Helper to run process_fd on a temp file with given content
static void integration_test(const char *content,
                             uint64_t exp_lines,
                             uint64_t exp_words,
                             uint64_t exp_bytes) {
    char fn[] = "/tmp/wc_test_XXXXXX";
    int fd = mkstemp(fn);
    assert(fd >= 0);
    write(fd, content, strlen(content));
    lseek(fd, 0, SEEK_SET);
    struct stats s = {0,0,0};
    int rc = process_fd(fd, &s);
    assert(rc == 0);
    assert(s.lines == exp_lines);
    assert(s.words == exp_words);
    assert(s.bytes == exp_bytes);
    close(fd);
    unlink(fn);
}

// Unit tests for count_buffer()
static void unit_tests() {
    struct stats s;

    // Empty string
    memset(&s,0,sizeof(s));
    count_buffer("",0,&s);
    // no trailing word
    assert(s.lines == 0 && s.words == 0 && s.bytes == 0);

    // Single word, no newline
    memset(&s,0,sizeof(s));
    count_buffer("hello",5,&s);
    // finalize trailing word
    if (5 > 0 && !is_ascii_space('o')) s.words++;
    assert(s.lines == 0 && s.words == 1 && s.bytes == 5);

    // Spaces only
    memset(&s,0,sizeof(s));
    count_buffer("   \t\n",5,&s);
    if (5 > 0 && !is_ascii_space('\n')) s.words++;
    assert(s.lines == 1 && s.words == 0 && s.bytes == 5);

    // Multiple words
    const char *txt = " foo  bar\nbaz\tqux ";
    memset(&s,0,sizeof(s));
    count_buffer(txt, strlen(txt), &s);
    // trailing space => no extra
    assert(s.lines == 1);
    // words: foo,bar,baz,qux
    assert(s.words == 4);
    assert(s.bytes == strlen(txt));
}

// Integration tests
static void run_integration() {
    integration_test("", 0, 0, 0);
    integration_test("one two three", 0, 3, 13);
    integration_test("line1\nline2\n", 2, 2, 12);
    integration_test("multi\n\nnewline\n", 3, 2, 15);
    integration_test("tab\tseparated\twords", 0, 3, 21);
}

// Simple performance test: process a big buffer N times
static void perf_test() {
    const size_t chunk = 10 * 1024 * 1024; // 10 MiB
    char *buf = malloc(chunk);
    // fill with 'a' and newline every 64 bytes
    for (size_t i = 0; i < chunk; i++) {
        buf[i] = (i % 64 == 0 ? '\n' : 'a');
    }
    struct stats s = {0,0,0};
    const int reps = 10;
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < reps; i++) {
        count_buffer(buf, chunk, &s);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double sec = (t1.tv_sec - t0.tv_sec)
               + (t1.tv_nsec - t0.tv_nsec) * 1e-9;
    double total_mb = (chunk * reps) / (1024.0 * 1024.0);
    printf("\nPerformance: %.2f MiB in %.3f s -> %.2f MiB/s\n",
           total_mb, sec, total_mb / sec);
    free(buf);
}

int main(void) {
    printf("Running unit tests...\n");
    unit_tests();
    printf("Unit tests passed.\n");

    printf("Running integration tests...\n");
    run_integration();
    printf("Integration tests passed.\n");

    printf("Running performance test...\n");
    perf_test();

    return 0;
}

