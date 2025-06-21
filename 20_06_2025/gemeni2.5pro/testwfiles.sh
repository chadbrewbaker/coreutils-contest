# First, create the large test file (takes a moment)
echo "Creating 1GB test file..."
mkfile -n 1g large_test_file.txt
# Fill it with some text-like data to make word counting non-trivial
head -c 500M /dev/urandom | base64 > large_test_file.txt

# Build the scalar and vectorized versions
make

# Now, run the benchmarks. We clear buffer cache on macOS between runs for a fairer I/O comparison.
# Note: `sudo purge` requires admin privileges.

echo -e "\n--- BENCHMARKING (1GB file) ---"

echo -e "\n1. System wc"
sudo purge; time wc large_test_file.txt

echo -e "\n2. Our C version (Scalar only)"
sudo purge; time ./fast_wc_scalar large_test_file.txt

echo -e "\n3. Our C version (NEON Vectorized)"
sudo purge; time ./fast_wc large_test_file.txt

# Clean up
rm large_test_file.txt
