# Makefile for fast_wc on macOS
CC = clang
# -O3: Aggressive optimization
# -march=native: Use all instructions available on the compiling machine (e.g., M1 NEON)
# -Wall -Wextra: Show all reasonable warnings
CFLAGS = -O3 -march=native -Wall -Wextra

# Target executables
TARGET = fast_wc
TARGET_SCALAR = fast_wc_scalar
TEST_TARGET = test_wc

all: $(TARGET) $(TARGET_SCALAR)

# Optimized build with NEON SIMD enabled
$(TARGET): fast_wc.c
	$(CC) $(CFLAGS) -o $@ $<

# Scalar-only build for performance comparison
$(TARGET_SCALAR): fast_wc.c
	$(CC) $(CFLAGS) -DNO_ARM_NEON -o $@ $<

# Build for unit tests
test_wc_obj: test_wc.c
	$(CC) $(CFLAGS) -c -o $@.o $<

test: test_wc_obj
	$(CC) $(CFLAGS) -o $(TEST_TARGET) test_wc_obj.o
	./$(TEST_TARGET)

# Run integration tests
integration-test: $(TARGET)
	@echo "Running integration tests..."
	@/bin/bash ./test_wc.sh

clean:
	rm -f $(TARGET) $(TARGET_SCALAR) $(TEST_TARGET) *.o
