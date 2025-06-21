# Efficient WC Utility for Mac M1

A high-performance implementation of the Unix `wc` (word count) utility, optimized for Apple Silicon (M1/M2) processors with ARM NEON SIMD instructions.

## Features

- **SIMD Optimization**: Uses ARM NEON instructions for fast line counting
- **Memory Efficient**: Uses memory mapping for large files, buffered reading for streams
- **Full Compatibility**: Drop-in replacement for system `wc` with identical output
- **Comprehensive Testing**: Extensive unit, integration, performance, and stress tests
- **Error Handling**: Robust error handling for various edge cases
- **Multiple File Support**: Handles single files, multiple files, and stdin

## Performance

Optimized for Mac M1 with:
- ARM64 architecture targeting (`-march=armv8-a+simd`)
- Apple M1 specific tuning (`-mtune=apple-m1`)
- Link-time optimization (`-flto`)
- SIMD vectorization for line counting
- Memory mapping for large files (>4KB)

## Build Instructions

### Prerequisites
- macOS with Apple Silicon (M1/M2)
- Clang compiler
- Make
- Python 3 (for some tests)

### Quick Build
```bash
make
```

### Build Options
```bash
make all           # Build optimized version (default)
make debug         # Build debug version with symbols
make clean         # Remove build artifacts
make install       # Install to /usr/local/bin
make uninstall     # Remove from /usr/local/bin
```

## Testing

### Run All Tests
```bash
make test          # Run comprehensive test suite
./test_wc.sh       # Run the comprehensive test script
```

### Individual Test Categories
```bash
make unit-tests         # Unit tests for core functions
make integration-tests  # Integration tests
make performance-tests  # Performance benchmarks
make stress-tests       # Stress tests with extreme inputs
make corner-cases       # Edge case testing
make compare           # Compare with system wc
make benchmark         # Performance vs system wc
```

### Comprehensive Testing
```bash
make test-suite        # Full test suite with all file types
make ci-test          # CI/CD test pipeline
```

## Usage

The utility supports all standard `wc` options:

```bash
# Count lines, words, and bytes (default)
./wc file.txt

# Count lines only
./wc -l file.txt

# Count words only  
./wc -w file.txt

# Count bytes only
./wc -c file.txt

# Count characters only
./wc -m file.txt

# Multiple options
./wc -lw file.txt      # Lines and words
./wc -lwc file.txt     # Lines, words, and bytes

# Multiple files
./wc file1.txt file2.txt file3.txt

# From stdin
cat file.txt | ./wc
echo "hello world" | ./wc

# Help and version
./wc --help
./wc --version
```

## Architecture

### Core Components

1. **SIMD Line Counting** (`count_lines_simd`)
   - Uses ARM NEON 128-bit vector instructions
   - Processes 16 bytes at a time
   - Fallback to scalar for remainder and non-NEON systems

2. **Optimized Word Counting** (`count_words_optimized`)
   - State machine approach for word boundaries
   - Single pass through data
   - Handles all standard whitespace characters

3. **Memory Management**
   - Memory mapping for large regular files (>4KB)
   - Buffered reading for stdin and small files
   - Efficient memory usage for very large files

4. **Error Handling**
   - Comprehensive error checking
   - Graceful handling of permission issues
   - Proper cleanup on failures

### Performance Optimizations

- **Compiler Optimizations**: `-O3
