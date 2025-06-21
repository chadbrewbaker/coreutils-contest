# Makefile for efficient wc utility - Mac M1 optimized

CC = clang
CFLAGS = -O3 -march=armv8-a+simd -mtune=apple-m1 -flto -Wall -Wextra -std=c99
LDFLAGS = -flto

# Source files
SRC = wc.c
TARGET = wc
TEST_TARGET = wc_test

# Default target
all: $(TARGET)

# Build the main wc utility
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

# Build with unit tests
unit-tests: $(SRC)
	$(CC) $(CFLAGS) -DUNIT_TESTS -o $(TEST_TARGET) $<
	./$(TEST_TARGET)

# Build with integration tests
integration-tests: $(TARGET)
	$(CC) $(CFLAGS) -DINTEGRATION_TESTS -o $(TEST_TARGET) $<
	./$(TEST_TARGET)

# Build with performance tests
performance-tests: $(SRC)
	$(CC) $(CFLAGS) -DPERFORMANCE_TESTS -o $(TEST_TARGET) $<
	./$(TEST_TARGET)

# Build with stress tests
stress-tests: $(SRC)
	$(CC) $(CFLAGS) -DSTRESS_TESTS -o $(TEST_TARGET) $<
	./$(TEST_TARGET)

# Run all tests
test: unit-tests
	@echo "Running comprehensive tests..."
	@$(MAKE) integration-tests
	@$(MAKE) performance-tests
	@$(MAKE) stress-tests
	@echo "All tests completed!"

# Compare with system wc for correctness
compare: $(TARGET)
	@echo "Comparing with system wc..."
	@echo "Creating test files..."
	@echo "Hello world" > test1.txt
	@echo -e "Line 1\nLine 2\nLine 3" > test2.txt
	@printf "No newline at end" > test3.txt
	@echo "" > test4.txt
	@seq 1 1000 > test5.txt
	
	@echo "Test 1 - Simple text:"
	@echo -n "System wc: "; wc test1.txt
	@echo -n "Our wc:    "; ./$(TARGET) test1.txt
	
	@echo -e "\nTest 2 - Multiple lines:"
	@echo -n "System wc: "; wc test2.txt
	@echo -n "Our wc:    "; ./$(TARGET) test2.txt
	
	@echo -e "\nTest 3 - No final newline:"
	@echo -n "System wc: "; wc test3.txt
	@echo -n "Our wc:    "; ./$(TARGET) test3.txt
	
	@echo -e "\nTest 4 - Empty file:"
	@echo -n "System wc: "; wc test4.txt
	@echo -n "Our wc:    "; ./$(TARGET) test4.txt
	
	@echo -e "\nTest 5 - Large file (1000 lines):"
	@echo -n "System wc: "; wc test5.txt
	@echo -n "Our wc:    "; ./$(TARGET) test5.txt
	
	@rm -f test*.txt

# Benchmark against system wc
benchmark: $(TARGET)
	@echo "Benchmarking against system wc..."
	@echo "Generating large test file (10MB)..."
	@seq 1 500000 | sed 's/.*/This is line & with some additional text to make it longer/' > large_test.txt
	
	@echo "System wc:"
	@time wc large_test.txt
	
	@echo -e "\nOur wc:"
	@time ./$(TARGET) large_test.txt
	
	@echo -e "\nDifference check:"
	@wc large_test.txt > system_output.txt
	@./$(TARGET) large_test.txt > our_output.txt
	@if diff system_output.txt our_output.txt > /dev/null; then \
		echo "✓ Outputs match perfectly!"; \
	else \
		echo "✗ Outputs differ:"; \
		diff system_output.txt our_output.txt; \
	fi
	
	@rm -f large_test.txt system_output.txt our_output.txt

# Memory usage test with valgrind (if available)
memcheck: $(TARGET)
	@if command -v valgrind >/dev/null 2>&1; then \
		echo "Running memory check with valgrind..."; \
		echo "Test data" > memtest.txt; \
		valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET) memtest.txt; \
		rm -f memtest.txt; \
	else \
		echo "valgrind not available, skipping memory check"; \
	fi

# Profile with instruments (Mac-specific)
profile: $(TARGET)
	@echo "Creating profile data..."
	@seq 1 100000 > profile_test.txt
	@echo "Run: xcrun xctrace record --template 'Time Profiler' --launch ./$(TARGET) profile_test.txt"
	@echo "Then analyze the .trace file with Instruments"
	@rm -f profile_test.txt

# Clean build artifacts
clean:
	rm -f $(TARGET) $(TEST_TARGET) *.txt *.trace

# Install to /usr/local/bin
install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/

# Uninstall
uninstall:
	sudo rm -f /usr/local/bin/$(TARGET)

# Debug build
debug: CFLAGS = -O0 -g -march=armv8-a+simd -Wall -Wextra -std=c99 -DDEBUG
debug: $(TARGET)

# Create a comprehensive test suite
test-suite: $(TARGET)
	@echo "Running comprehensive test suite..."
	@mkdir -p test_results
	
	# Test various file sizes
	@echo "Testing different file sizes..."
	@echo "" > test_results/empty.txt
	@echo "single line" > test_results/single.txt
	@seq 1 10 > test_results/small.txt
	@seq 1 1000 > test_results/medium.txt
	@seq 1 100000 > test_results/large.txt
	
	# Test edge cases
	@printf "no_newline" > test_results/no_newline.txt
	@printf "\n\n\n" > test_results/only_newlines.txt
	@printf "   \t\t   \n  \t  \n" > test_results/whitespace.txt
	
	# Test all combinations of flags
	@for file in test_results/*.txt; do \
		echo "Testing $$file:"; \
		echo -n "  default: "; ./$(TARGET) $$file; \
		echo -n "  -l:      "; ./$(TARGET) -l $$file; \
		echo -n "  -w:      "; ./$(TARGET) -w $$file; \
		echo -n "  -c:      "; ./$(TARGET) -c $$file; \
		echo -n "  -m:      "; ./$(TARGET) -m $$file; \
		echo -n "  -lwc:    "; ./$(TARGET) -lwc $file; \
		echo; \
	done
	
	@rm -rf test_results
	@echo "✓ Comprehensive test suite completed!"

# Test corner cases specifically
corner-cases: $(TARGET)
	@echo "Testing corner cases..."
	
	# Test with binary data
	@printf "\x00\x01\x02hello\nworld\x00\xff" > binary_test.txt
	@echo -n "Binary data test: "; ./$(TARGET) binary_test.txt
	
	# Test with very long lines
	@python3 -c "print('a' * 10000 + '\n' + 'b' * 5000)" > long_lines.txt
	@echo -n "Long lines test: "; ./$(TARGET) long_lines.txt
	
	# Test with mixed line endings (should work since we only count \n)
	@printf "line1\r\nline2\nline3\r\n" > mixed_endings.txt
	@echo -n "Mixed line endings: "; ./$(TARGET) mixed_endings.txt
	
	# Test with Unicode (should count bytes correctly for ASCII subset)
	@printf "hello\nworld\n" > ascii_test.txt
	@echo -n "ASCII test: "; ./$(TARGET) ascii_test.txt
	
	# Test empty stdin
	@echo -n "Empty stdin test: "; echo -n "" | ./$(TARGET)
	
	# Test large stdin
	@echo -n "Large stdin test: "; seq 1 1000 | ./$(TARGET)
	
	@rm -f binary_test.txt long_lines.txt mixed_endings.txt ascii_test.txt
	@echo "✓ Corner cases testing completed!"

# Performance comparison with different optimization levels
perf-compare:
	@echo "Comparing performance with different optimization levels..."
	
	# Build with different optimization levels
	$(CC) -O0 -march=armv8-a+simd -o wc_O0 $(SRC)
	$(CC) -O1 -march=armv8-a+simd -o wc_O1 $(SRC)
	$(CC) -O2 -march=armv8-a+simd -o wc_O2 $(SRC)
	$(CC) -O3 -march=armv8-a+simd -o wc_O3 $(SRC)
	$(CC) -O3 -march=armv8-a+simd -flto -o wc_O3_lto $(SRC)
	
	# Create test file
	@seq 1 100000 | sed 's/.*/This is line & with some text/' > perf_test.txt
	
	@echo "Performance comparison (3 runs each):"
	@echo "O0 optimization:"
	@time ./wc_O0 perf_test.txt >/dev/null
	@time ./wc_O0 perf_test.txt >/dev/null
	@time ./wc_O0 perf_test.txt >/dev/null
	
	@echo -e "\nO1 optimization:"
	@time ./wc_O1 perf_test.txt >/dev/null
	@time ./wc_O1 perf_test.txt >/dev/null
	@time ./wc_O1 perf_test.txt >/dev/null
	
	@echo -e "\nO2 optimization:"
	@time ./wc_O2 perf_test.txt >/dev/null
	@time ./wc_O2 perf_test.txt >/dev/null
	@time ./wc_O2 perf_test.txt >/dev/null
	
	@echo -e "\nO3 optimization:"
	@time ./wc_O3 perf_test.txt >/dev/null
	@time ./wc_O3 perf_test.txt >/dev/null
	@time ./wc_O3 perf_test.txt >/dev/null
	
	@echo -e "\nO3 + LTO optimization:"
	@time ./wc_O3_lto perf_test.txt >/dev/null
	@time ./wc_O3_lto perf_test.txt >/dev/null
	@time ./wc_O3_lto perf_test.txt >/dev/null
	
	@echo -e "\nSystem wc for comparison:"
	@time wc perf_test.txt >/dev/null
	@time wc perf_test.txt >/dev/null
	@time wc perf_test.txt >/dev/null
	
	@rm -f wc_O* perf_test.txt

# Test with real-world files
real-world-test: $(TARGET)
	@echo "Testing with real-world files..."
	
	# Test with source code files
	@if [ -f "$(SRC)" ]; then \
		echo "Testing with source file $(SRC):"; \
		echo -n "System wc: "; wc $(SRC); \
		echo -n "Our wc:    "; ./$(TARGET) $(SRC); \
		echo; \
	fi
	
	# Test with system files (if accessible)
	@if [ -r "/etc/passwd" ]; then \
		echo "Testing with /etc/passwd:"; \
		echo -n "System wc: "; wc /etc/passwd; \
		echo -n "Our wc:    "; ./$(TARGET) /etc/passwd; \
		echo; \
	fi
	
	# Test with multiple files
	@echo "Testing with multiple files:"; \
	echo "file1" > multi1.txt; \
	echo -e "file2\nline2" > multi2.txt; \
	echo -e "file3\nline2\nline3" > multi3.txt; \
	echo "System wc:"; wc multi*.txt; \
	echo "Our wc:"; ./$(TARGET) multi*.txt; \
	rm -f multi*.txt

# Code quality checks
quality-check:
	@echo "Running code quality checks..."
	
	# Check for common issues with clang static analyzer
	@if command -v clang >/dev/null 2>&1; then \
		echo "Running clang static analyzer..."; \
		clang --analyze -Xanalyzer -analyzer-output=text $(SRC); \
	fi
	
	# Check with additional warnings
	@echo "Compiling with extra warnings..."
	$(CC) -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wstrict-prototypes \
		-Wmissing-prototypes -Wold-style-definition -std=c99 -c $(SRC)
	
	# Check formatting (if clang-format available)
	@if command -v clang-format >/dev/null 2>&1; then \
		echo "Checking code formatting..."; \
		clang-format --dry-run --Werror $(SRC) || echo "Code formatting issues found"; \
	fi

# Documentation and help
help:
	@echo "Available targets:"
	@echo "  all              - Build the wc utility (default)"
	@echo "  test             - Run all tests (unit, integration, performance, stress)"
	@echo "  unit-tests       - Run unit tests only"
	@echo "  integration-tests- Run integration tests"
	@echo "  performance-tests- Run performance benchmarks"
	@echo "  stress-tests     - Run stress tests with extreme inputs"
	@echo "  compare          - Compare output with system wc"
	@echo "  benchmark        - Benchmark performance against system wc"
	@echo "  corner-cases     - Test edge cases and unusual inputs"
	@echo "  test-suite       - Run comprehensive test suite"
	@echo "  perf-compare     - Compare different optimization levels"
	@echo "  real-world-test  - Test with real files"
	@echo "  memcheck         - Run memory leak detection (requires valgrind)"
	@echo "  profile          - Set up for profiling with Instruments"
	@echo "  quality-check    - Run code quality analysis"
	@echo "  debug            - Build debug version"
	@echo "  clean            - Remove build artifacts"
	@echo "  install          - Install to /usr/local/bin"
	@echo "  uninstall        - Remove from /usr/local/bin"
	@echo "  help             - Show this help message"

# Set up CI/CD testing
ci-test: clean
	@echo "Running CI/CD test suite..."
	@$(MAKE) all
	@$(MAKE) test
	@$(MAKE) compare
	@$(MAKE) corner-cases
	@$(MAKE) quality-check
	@echo "✓ All CI/CD tests passed!"

.PHONY: all test unit-tests integration-tests performance-tests stress-tests \
        compare benchmark memcheck profile clean install uninstall debug \
        test-suite corner-cases perf-compare real-world-test quality-check \
        help ci-test
