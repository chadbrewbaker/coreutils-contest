// wc.c
#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

struct stats {
    uint64_t lines;
    uint64_t words;
    uint64_t bytes;
};

// Return non‐zero if ASCII whitespace (space, \n, \t, \v, \f, \r)
static inline int is_ascii_space(char c) {
    // ' ' == 0x20, '\t'==0x09, '\n'==0x0A, '\v'==0x0B, '\f'==0x0C, '\r'==0x0D
    return (unsigned)(c - 9) < (13 - 9 + 1) || c == ' ';
}

// Process a single buffer, update stats.
void count_buffer(const char *buf, size_t len, struct stats *s) {
    uint64_t in_word = 0;
    s->bytes += len;
    for (size_t i = 0; i < len; i++) {
        char c = buf[i];
        if (c == '\n') {
            s->lines++;
        }
        int sp = is_ascii_space(c);
        // Transition from non-space to space ends a word
        if (!sp) {
            in_word = 1;
        } else if (in_word) {
            s->words++;
            in_word = 0;
        }
    }
    // If buffer ends in a word, we defer counting until next chunk or end‐of‐file.
    // Caller must finalize.
}

// Process one file descriptor
int process_fd(int fd, struct stats *s) {
    enum { BUF_SIZE = 1 << 20 }; // 1 MiB
    char *buf = malloc(BUF_SIZE);
    if (!buf) {
        perror("malloc");
        return -1;
    }
    struct stats local = {0,0,0};
    ssize_t r;
    while ((r = read(fd, buf, BUF_SIZE)) > 0) {
        count_buffer(buf, (size_t)r, &local);
    }
    if (r < 0) {
        perror("read");
        free(buf);
        return -1;
    }
    // If file did not end in whitespace, we have one trailing word
    // Check last byte:
    if (local.bytes > 0) {
        char last = buf[(size_t)( (local.bytes - 1) % BUF_SIZE )];
        if (!is_ascii_space(last))
            local.words++;
    }
    *s = local;
    free(buf);
    return 0;
}

int main(int argc, char *argv[]) {
    struct stats total = {0,0,0};
    int files = 0;
    if (argc == 1) {
        struct stats s;
        if (process_fd(STDIN_FILENO, &s) != 0) return 1;
        printf("%8" PRIu64 "%8" PRIu64 "%8" PRIu64 "\n",
               s.lines, s.words, s.bytes);
    } else {
        for (int i = 1; i < argc; i++) {
            int fd = open(argv[i], O_RDONLY);
            if (fd < 0) {
                fprintf(stderr, "wc: cannot open '%s': %s\n",
                        argv[i], strerror(errno));
                continue;
            }
            struct stats s = {0,0,0};
            if (process_fd(fd, &s) == 0) {
                printf("%8" PRIu64 "%8" PRIu64 "%8" PRIu64 " %s\n",
                       s.lines, s.words, s.bytes, argv[i]);
                total.lines  += s.lines;
                total.words  += s.words;
                total.bytes  += s.bytes;
                files++;
            }
            close(fd);
        }
        if (files > 1) {
            printf("%8" PRIu64 "%8" PRIu64 "%8" PRIu64 " total\n",
                   total.lines, total.words, total.bytes);
        }
    }
    return 0;
}

