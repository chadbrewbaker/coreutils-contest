Before diving into the code, let me explain my approach and assumptions:

Functionality
The classic wc utility counts lines, words, and bytes. We’ll replicate that behavior exactly for ASCII input.
Performance on M1
We’ll use a large (1 MiB) read buffer to amortize syscall overhead.
We’ll scan bytes with a tight, branch‐minimized loop, manually inlining the ASCII‐whitespace check rather than calling isspace().
We’ll use read() and POSIX APIs only—clang/LLVM on macOS will auto-vectorize where appropriate.
Structure for testability
Split out a count_buffer() function that accepts a buffer + length and updates a stats struct.
main() opens each file (or STDIN), reads in chunks, and invokes count_buffer().
Testing
Unit tests call count_buffer() directly on literal strings to cover corner cases: empty input, multiple whitespace, no trailing newline, etc.
Integration tests open small temporary files with known contents, call process_file(), and check that the totals match the expected line/word/byte counts.
Performance test builds a large in-memory buffer (~100 MiB of repeating pattern) and measures how long it takes to process it repeatedly (to give a rough MB/sec figure).
All code compiles with clang -O3 -Wall -Wextra. Below are two source files: wc.c (the utility) and test_wc.c (tests). You can compile them together:

clang -O3 -std=c11 wc.c test_wc.c -o wc_test && ./wc_test


How to build and run

clang -O3 -std=c11 wc.c test_wc.c -o wc_test
./wc_test
You should see:
