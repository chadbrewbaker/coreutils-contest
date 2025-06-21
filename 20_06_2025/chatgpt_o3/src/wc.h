###############################################################################
# src/wc.h
#ifndef WC_H
#define WC_H
#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint64_t lines;
    uint64_t words;
    uint64_t bytes;
} wc_counts_t;

void wc_count_buffer(const uint8_t *data, size_t len, wc_counts_t *c);

#endif // WC_H
