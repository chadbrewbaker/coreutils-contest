// test_wc.c
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

// We need to declare the functions and structs from fast_wc.c to use them here.
// In a larger project, these would be in a header file.
typedef struct {
    long lines;
    long words;
    long bytes;
} Counts;

// Declaration of the function we are testing
bool process_buffer(const unsigned char* buffer, size_t size, Counts* counts, bool in_word_prev);

void run_test(const char* name, const char* input, bool in_word_prev, 
              long exp_lines, long exp_words, long exp_bytes, bool exp_in_word_final) {
    printf("Running test: %s... ", name);
    Counts counts = {0, 0, 0};
    bool final_in_word = process_buffer((const unsigned char*)input, strlen(input), &counts, in_word_prev);
    
    assert(counts.lines == exp_lines);
    assert(counts.words == exp_words);
    assert(counts.bytes == exp_bytes);
    assert(final_in_word == exp_in_word_final);
    
    printf("PASSED\n");
}

int main() {
    // Test 1: Simple case
    run_test("Simple", "hello world\n", false, 1, 2, 12, false);

    // Test 2: Empty input
    run_test("Empty", "", false, 0, 0, 0, false);

    // Test 3: No newlines
    run_test("No Newline", "one two three", false, 0, 3, 13, true);

    // Test 4: Leading/trailing/multiple spaces
    run_test("Spaces", "  word1  word2 \n", false, 1, 2, 17, false);
    
    // Test 5: Only newlines and spaces
    run_test("Whitespace Only", " \n \n ", false, 2, 0, 5, false);

    // Test 6: Word boundary start (in_word_prev = true)
    run_test("Word Boundary Start", " word", true, 0, 0, 5, true);
    
    // Test 7: Word boundary continues (in_word_prev = true)
    run_test("Word Boundary Continue", "word", true, 0, 0, 4, true);

    // Test 8: Word boundary ends (in_word_prev = true)
    run_test("Word Boundary End", " ", true, 0, 0, 1, false);

    // Test 9: More than 16 chars to trigger SIMD loop
    run_test("Long String SIMD", "this is a line\nand another\n", false, 2, 6, 29, false);

    printf("\nAll unit tests passed!\n");
    return 0;
}
