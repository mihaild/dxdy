#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <algorithm>
#include <fstream>
#include <csignal>
#include <cmath>
#include <iomanip>

// CUDA runtime
#include <cuda_runtime.h>

#define RESET   "\033[0m"
#define BLUE    "\033[34m"      /* Blue */

// State file to store progress
const char* PROGRESS_FILE = "prime_progress.dat";
const char* LOG_FILE = "good_primes.log";

// Global flag for graceful shutdown
volatile bool g_keep_running = true;

// --- Utility and State Management ---

// Handles Ctrl+C to allow for a graceful shutdown
void signal_handler(int signum) {
    if (signum == SIGINT) {
        std::cerr << "\nCaught signal " << signum << ". Shutting down gracefully..." << std::endl;
        g_keep_running = false;
    }
}

// Struct to hold the application's state for persistence
struct ProgressState {
    uint64_t first_unchecked = 0;
    double total_duration_seconds = 0.0;
};

// Saves the current progress to the state file
void save_progress(const ProgressState& state) {
    std::ofstream file(PROGRESS_FILE, std::ios::binary);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(&state), sizeof(state));
    } else {
        std::cerr << "Error: Could not open " << PROGRESS_FILE << " for writing." << std::endl;
    }
}

// Loads progress from the state file. If not found, returns a default state.
ProgressState load_progress() {
    ProgressState state;
    std::ifstream file(PROGRESS_FILE, std::ios::binary);
    if (file.is_open()) {
        file.read(reinterpret_cast<char*>(&state), sizeof(state));
        std::cerr << "Resuming progress. First unchecked: " << state.first_unchecked
                  << ", Total time so far: " << state.total_duration_seconds << "s." << std::endl;
    } else {
        std::cerr << "No progress file found. Starting from scratch." << std::endl;
    }
    return state;
}

// Macro for checking CUDA errors
#define gpuErrchk(ans) { gpuAssert((ans), __FILE__, __LINE__); }
inline void gpuAssert(cudaError_t code, const char *file, int line, bool abort = true) {
    if (code != cudaSuccess) {
        fprintf(stderr, "GPUassert: %s %s %d\n", cudaGetErrorString(code), file, line);
        if (abort) exit(code);
    }
}

std::vector<uint64_t> generate_primes(const std::vector<uint64_t>& base_primes, uint64_t lower_bound, uint64_t upper_bound) {
    std::vector<bool> is_prime(upper_bound - lower_bound, true);
    for (const uint64_t p : base_primes) {
        if (p * p > upper_bound) {
            break;
        }
        for (uint64_t j = std::max(p * p, (lower_bound + p - 1) / p * p); j < upper_bound; j += p) {
            is_prime[j - lower_bound] = false;
        }
    }
    std::vector<uint64_t> result;
    for (uint64_t i = 0; i < is_prime.size(); ++i) {
        if (is_prime[i]) {
            result.push_back(i + lower_bound);
        }
    }
    return result;
}


// --- CUDA Kernel ---

/**
 * @brief CUDA kernel to process a block of prime numbers.
 * Each GPU thread processes one prime number independently.
 * @param p_dev          Device pointer to the array of primes in the current block.
 * @param s_out_dev      Device pointer to store the final 's' value for each prime.
 * @param zeroes_out_dev Device pointer to store the count of 'zeroes' for each prime.
 * @param block_size     The number of primes in the current block.
 */
__global__ void process_block_kernel(const uint64_t* p_dev, uint64_t* s_out_dev, int* zeroes_out_dev, size_t block_size) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= block_size) return;

    uint64_t p = p_dev[i];
    uint64_t r = 1;
    uint64_t s = 1;
    int zeroes = 0;

    // Use fast modular multiplication for (r * n) % p
    __int128 m = uint64_t(-1) / p;
    for (uint64_t n = 2; n < p; ++n) {
        r = r * n;
        uint64_t q = (m * r) >> 64;
        uint64_t t = r - q * p;
        r = t - p * (t >= p);
        s += r;
        if (s >= p) s -= p;

        if (s == 0) {
            zeroes++;
        }
    }

    s_out_dev[i] = s;
    zeroes_out_dev[i] = zeroes;
}

// --- Main Application Logic ---

int main() {
    // Register signal handler for graceful shutdown
    signal(SIGINT, signal_handler);

    // Load previous state or start new
    ProgressState state = load_progress();
    auto session_start_time = std::chrono::steady_clock::now();

    // Open log file in append mode
    std::ofstream log_file(LOG_FILE, std::ios_base::app);
    if (!log_file.is_open()) {
        std::cerr << "Error: Could not open log file " << LOG_FILE << std::endl;
        return 1;
    }

    // Segment size for generating primes. Tune for performance vs. memory.
    const uint64_t SEGMENT_SIZE = 1024 * 1024 * 4;

    const uint64_t BASE_PRIMES = 1000000;
    std::vector<uint64_t> base_primes;
    {
        std::vector<bool> is_prime(BASE_PRIMES, true);
        for (uint64_t p = 2; p < BASE_PRIMES; ++p) {
            if (is_prime[p]) {
                base_primes.push_back(p);
                for (uint64_t j = p * p; j < BASE_PRIMES; j += p) {
                    is_prime[j] = false;
                }
            }
        }
    }

    uint64_t* p_dev = nullptr;
    uint64_t* s_dev = nullptr;
    int* zeroes_dev = nullptr;

    gpuErrchk(cudaMalloc(&p_dev, SEGMENT_SIZE * sizeof(uint64_t)));
    gpuErrchk(cudaMalloc(&s_dev, SEGMENT_SIZE * sizeof(uint64_t)));
    gpuErrchk(cudaMalloc(&zeroes_dev, SEGMENT_SIZE * sizeof(int)));

    // --- Main processing loop ---
    while (g_keep_running) {
        uint64_t lower_bound = state.first_unchecked;
        uint64_t upper_bound = state.first_unchecked + SEGMENT_SIZE;
        if (upper_bound >= BASE_PRIMES * BASE_PRIMES) {
            std::cerr << "Not enough primes precalculated" << std::endl;
            std::terminate();
        }

        std::vector<uint64_t> primes_to_check = generate_primes(base_primes, lower_bound, upper_bound);

        if (primes_to_check.empty()) {
            state.first_unchecked = upper_bound; // Skip empty segments
            continue;
        }

        std::cerr << BLUE << "Checking " << primes_to_check.size() << " primes in range ["
                  << primes_to_check.front() << ", " << primes_to_check.back() << "]" << RESET << std::endl;

        // --- CUDA Processing for the current segment ---
        size_t num_primes = primes_to_check.size();
        std::vector<uint64_t> s_host(num_primes);
        std::vector<int> zeroes_host(num_primes);

        gpuErrchk(cudaMemcpy(p_dev, primes_to_check.data(), num_primes * sizeof(uint64_t), cudaMemcpyHostToDevice));

        const int threads_per_block = 256;
        const int blocks_per_grid = (num_primes + threads_per_block - 1) / threads_per_block;
        process_block_kernel<<<blocks_per_grid, threads_per_block>>>(p_dev, s_dev, zeroes_dev, num_primes);
        gpuErrchk(cudaGetLastError());
        gpuErrchk(cudaDeviceSynchronize());

        gpuErrchk(cudaMemcpy(s_host.data(), s_dev, num_primes * sizeof(uint64_t), cudaMemcpyDeviceToHost));
        gpuErrchk(cudaMemcpy(zeroes_host.data(), zeroes_dev, num_primes * sizeof(int), cudaMemcpyDeviceToHost));

        // --- Process and Log Results ---
        for (size_t i = 0; i < num_primes; ++i) {
            if (zeroes_host[i] >= 8 || s_host[i] == 0) {
                std::stringstream ss;
                ss << primes_to_check[i] << ' ' << (s_host[i] == 0) << ' ' << zeroes_host[i] << std::endl;
                std::cout << ss.str(); // Log to stdout
                log_file << ss.str();  // Log to file
            }
        }
        log_file.flush();

        // Update state for the next iteration
        state.first_unchecked = upper_bound;
        auto now = std::chrono::steady_clock::now();
        double session_elapsed = std::chrono::duration<double>(now - session_start_time).count();
        state.total_duration_seconds += session_elapsed;
        session_start_time = now;

        save_progress(state);
        std::cerr << BLUE << "Progress Update: first unchecked: " << state.first_unchecked
                  << ". Total time: " << std::fixed << std::setprecision(2) << state.total_duration_seconds << "s." << RESET << std::endl;
    }

    gpuErrchk(cudaFree(p_dev));
    gpuErrchk(cudaFree(s_dev));
    gpuErrchk(cudaFree(zeroes_dev));

    // Final save before exiting
    auto final_time = std::chrono::steady_clock::now();
    state.total_duration_seconds += std::chrono::duration<double>(final_time - session_start_time).count();
    save_progress(state);

    std::cout << "Application finished. Total time spent across all sessions: "
              << state.total_duration_seconds << " seconds." << std::endl;

    return 0;
}

