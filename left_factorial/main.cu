#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm> // For std::min

// CUDA runtime
#include <cuda_runtime.h>

#define RESET   "\033[0m"
#define BLUE    "\033[34m"      /* Blue */

// Macro for checking CUDA errors
#define gpuErrchk(ans) { gpuAssert((ans), __FILE__, __LINE__); }
inline void gpuAssert(cudaError_t code, const char *file, int line, bool abort=true)
{
   if (code != cudaSuccess)
   {
      fprintf(stderr,"GPUassert: %s %s %d\n", cudaGetErrorString(code), file, line);
      if (abort) exit(code);
   }
}

/**
 * @brief CUDA kernel to process a block of prime numbers.
 * * Each GPU thread processes one prime number independently. The loop for 'n' for each prime 'p'
 * runs from 2 up to p-1. This is an optimization over the original CPU code, as any
 * calculations for n >= p do not change the final result.
 * * @param p_dev         Device pointer to the array of primes in the current block.
 * @param s_out_dev     Device pointer to store the final 's' value for each prime.
 * @param zeroes_out_dev Device pointer to store the count of 'zeroes' for each prime.
 * @param block_size    The number of primes in the current block.
 */
__global__ void process_block_kernel(const uint64_t* p_dev, uint64_t* s_out_dev, int* zeroes_out_dev, size_t block_size) {
    // Calculate the global thread ID
    int i = blockIdx.x * blockDim.x + threadIdx.x;

    // Boundary check to ensure we don't process out of bounds
    if (i >= block_size) {
        return;
    }

    // Get the prime number for this thread
    uint64_t p = p_dev[i];

    // Local variables for the calculation
    uint64_t r = 1;
    uint64_t s = 1;
    int zeroes = 0;

    // The main computation loop for the prime 'p'.
    // We calculate s = (1! + 2! + ... + (p-1)!) mod p
    __int128 m = uint64_t(-1) / p;
    for (uint64_t n = 2; n < p; ++n) {
        // r becomes n! mod p
        //r = (r * n) % p;

        r = r * n;
        uint64_t q = (m * r) >> 64;
        uint64_t t = r - q * p;
        r = t - p * (t >= p);
        // s becomes the cumulative sum of factorials mod p
        s += r;
        if (s >= p) {
            s -= p;
        }
        if (s == 0) {
            zeroes++;
        }
    }

    // Store the final results back into global memory
    s_out_dev[i] = s;
    zeroes_out_dev[i] = zeroes;
}


int main() {
    uint64_t N;
    std::cout << "Enter the upper limit N to find primes: ";
    std::cin >> N;

    // --- Step 1: Sieve of Eratosthenes to find primes on the CPU ---
    std::vector<bool> is_prime(N, true);
    std::vector<uint64_t> primes;
    is_prime[0] = is_prime[1] = false;
    for (int i = 2; i < N; ++i) {
        if (is_prime[i]) {
            primes.push_back(i);
            for (int j = i + i; j < N; j += i) {
                is_prime[j] = false;
            }
        }
    }

    if (primes.empty()) {
        std::cerr << "No primes found up to " << N << std::endl;
        return 1;
    }
    std::cerr << "Will check " << primes.size() << " primes, up to " << primes.back() << std::endl;

    // --- Step 2: Set up for block processing and CUDA ---
    auto start = std::chrono::system_clock::now();
    auto last = start;

    // Size of blocks to process on the GPU at a time. Tune for performance.
    constexpr size_t BLOCK_SIZE = 1024 * 128;

    // For progress reporting
    std::vector<uint64_t> cum_sums(primes);
    for (size_t i = 1; i < cum_sums.size(); ++i) {
        cum_sums[i] += cum_sums[i - 1];
    }

    // Host vectors to hold results from the GPU for one block
    std::vector<uint64_t> s_host(BLOCK_SIZE);
    std::vector<int> zeroes_host(BLOCK_SIZE);

    // Device pointers
    uint64_t* p_dev = nullptr;
    uint64_t* s_dev = nullptr;
    int* zeroes_dev = nullptr;

    // Allocate memory on the GPU for one block
    gpuErrchk(cudaMalloc(&p_dev, BLOCK_SIZE * sizeof(uint64_t)));
    gpuErrchk(cudaMalloc(&s_dev, BLOCK_SIZE * sizeof(uint64_t)));
    gpuErrchk(cudaMalloc(&zeroes_dev, BLOCK_SIZE * sizeof(int)));

    // --- Step 3: Main loop to process primes in blocks ---
    for (size_t block_start = 0; block_start < primes.size(); block_start += BLOCK_SIZE) {
        // Progress reporting (similar to the original)
        auto cur = std::chrono::system_clock::now();
        if (std::chrono::duration<double>(cur - last).count() > 10) {
            double t = std::chrono::duration<double>(cur - start).count();
            double expected_total = t / cum_sums[block_start] * cum_sums.back();
            std::cerr << BLUE << "i=" << block_start << ", t=" << t << ", expected_total=" << expected_total << RESET << std::endl;
            last = cur;
        }

        const size_t current_block_size = std::min(BLOCK_SIZE, primes.size() - block_start);

        // Copy current block of primes from host to device
        gpuErrchk(cudaMemcpy(p_dev, primes.data() + block_start, current_block_size * sizeof(uint64_t), cudaMemcpyHostToDevice));

        // Configure and launch the CUDA kernel
        const int threads_per_block = 256;
        const int blocks_per_grid = (current_block_size + threads_per_block - 1) / threads_per_block;
        process_block_kernel<<<blocks_per_grid, threads_per_block>>>(p_dev, s_dev, zeroes_dev, current_block_size);

        // Check for any errors during kernel launch
        gpuErrchk(cudaGetLastError());
        // Wait for the kernel to complete
        gpuErrchk(cudaDeviceSynchronize());

        // Copy results from device back to host
        gpuErrchk(cudaMemcpy(s_host.data(), s_dev, current_block_size * sizeof(uint64_t), cudaMemcpyDeviceToHost));
        gpuErrchk(cudaMemcpy(zeroes_host.data(), zeroes_dev, current_block_size * sizeof(int), cudaMemcpyDeviceToHost));

        // Process results on the CPU
        for (size_t i = 0; i < current_block_size; ++i) {
            if (zeroes_host[i] >= 8 || s_host[i] == 0) {
                std::cout << primes[i + block_start] << ' ' << (s_host[i] == 0) << ' ' << zeroes_host[i] << std::endl;
            }
        }
    }

    // --- Step 4: Cleanup ---
    gpuErrchk(cudaFree(p_dev));
    gpuErrchk(cudaFree(s_dev));
    gpuErrchk(cudaFree(zeroes_dev));

    std::cout << "total time " << std::chrono::duration<double>(std::chrono::system_clock::now() - start).count() << std::endl;
    return 0;
}

