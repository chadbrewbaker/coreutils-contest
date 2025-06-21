# Makefile
CC = clang
CFLAGS = -O3 -march=armv8-a+simd -std=c11 -Wall -Wextra -pedantic
LDFLAGS =
SRC = src/wc.c
TEST_SRC = tests/test_wc.c
BENCH_SRC = benches/bench_wc.c

.PHONY: all test bench clean

all: wc

wc: $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

test: wc $(TEST_SRC)
	$(CC) $(CFLAGS) -o test_wc $(TEST_SRC)
	./test_wc

bench: wc $(BENCH_SRC)
	$(CC) $(CFLAGS) -o bench_wc $(BENCH_SRC)
	./bench_wc $(FILE)

clean:
	rm -f wc test_wc bench_wc
