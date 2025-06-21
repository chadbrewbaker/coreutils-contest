// fast_wc.c
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

// Include ARM NEON intrinsics header
#if defined(__ARM_NEON)
#include <arm_neon.h>
#endif

// A large buffer is key to performance. 128KB is a good starting point.
#define BUFFER_SIZE (128 * 1024)

// Struct to hold our counts
typedef struct {
    long lines;
    long words;
    long bytes;
} Counts;

// The core logic for processing a buffer of data.
// It takes the previous character's state to correctly handle words across buffer boundaries.
static inline bool process_buffer(const unsigned char* buffer, size_t size, Counts* counts, bool in_word_prev) {
    bool in_word = in_word_prev;

    // Byte count is trivial
    counts->bytes += size;

    // Pointers for loops
    const unsigned char* p = buffer;
    const unsigned char* end = buffer + size;
    
#if defined(__ARM_NEON)
    // SIMD implementation for newline counting (super fast)
    // We create a vector of all newline characters.
    uint8x16_t nl_vector = vdupq_n_u8('\n');
    uint64_t line_count_in_buffer = 0;

    // Process the buffer in 16-byte chunks
    const unsigned char* aligned_end = buffer + (size / 16) * 16;
    while (p < aligned_end) {
        // Load 16 bytes from memory
        uint8x16_t chunk = vld1q_u8(p);
        // Compare each byte with the newline character. Result is 0xFF for true, 0x00 for false.
        uint8x16_t result = vceqq_u8(chunk, nl_vector);
        // Right-shift by 7 bits to turn 0xFF into 0x01. This makes summing easier.
        result = vshrq_n_u8(result, 7);
        // Add up the 1s in the vector and add to our total.
        line_count_in_buffer += vaddvq_u8(result);
        p += 16;
    }
    counts->lines += line_count_in_buffer;
#endif

    // Scalar processing for the remainder (for newlines) and for all word counting.
    // We restart the pointer 'p' from the beginning for the word count logic.
    p = buffer;
    while (p < end) {
#if !defined(__ARM_NEON)
        // If not using NEON, do scalar newline counting here.
        if (*p == '\n') {
            counts->lines++;
        }
#endif
        // Word counting state machine
        if (isspace(*p)) {
            in_word = false;
        } else {
            if (!in_word) {
                counts->words++;
            }
            in_word = true;
        }
        p++;
    }

#if defined(__ARM_NEON)
    // Handle the few leftover bytes for newline counting that didn't fit in a 16-byte vector
    p = aligned_end;
    while (p < end) {
        if (*p == '\n') {
            counts->lines++;
        }
        p++;
    }
#endif

    return in_word;
}

void process_file(const char* filename, FILE* fp, Counts* total_counts) {
    Counts file_counts = {0, 0, 0};
    unsigned char buffer[BUFFER_SIZE];
    size_t bytes_read;
    bool in_word = false;

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
        in_word = process_buffer(buffer, bytes_read, &file_counts, in_word);
    }

    printf("%8ld %8ld %8ld %s\n", file_counts.lines, file_counts.words, file_counts.bytes, filename);
    total_counts->lines += file_counts.lines;
    total_counts->words += file_counts.words;
    total_counts->bytes += file_counts.bytes;
}

int main(int argc, char* argv[]) {
    // Disable buffering on stdout for more immediate output
    setvbuf(stdout, NULL, _IOLBF, 0);

    if (argc == 1) {
        // Process stdin
        Counts counts = {0, 0, 0};
        bool in_word = false;
        unsigned char buffer[BUFFER_SIZE];
        size_t bytes_read;

        while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, stdin)) > 0) {
            in_word = process_buffer(buffer, bytes_read, &counts, in_word);
        }
        printf("%8ld %8ld %8ld\n", counts.lines, counts.words, counts.bytes);
    } else {
        Counts total_counts = {0, 0, 0};
        for (int i = 1; i < argc; i++) {
            FILE* fp = fopen(argv[i], "rb");
            if (fp == NULL) {
                perror(argv[i]);
                continue;
            }
            process_file(argv[i], fp, &total_counts);
            fclose(fp);
        }

        if (argc > 2) {
            printf("%8ld %8ld %8ld total\n", total_counts.lines, total_counts.words, total_counts.bytes);
        }
    }

    return 0;
}
