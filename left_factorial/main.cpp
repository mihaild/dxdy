#include <omp.h>
#include <array>
#include <iostream>
#include <vector>
#include <chrono>

#define RESET   "\033[0m"
#define BLUE    "\033[34m"      /* Blue */

void process_block(const int64_t* p, int64_t* r, int64_t* s, int64_t* zeroes, size_t size) {
    std::fill(r, r + size, 1);
    std::fill(s, s + size, 1);
    std::fill(zeroes, zeroes + size, 0);

    size_t shift = 0;
    for (int64_t n = 2; n < *(p + size - 1); ++n) {
        while (p[shift] <= n) {
            ++shift;
        }
        for (size_t i = shift; i < size; ++i) {
            r[i] = (r[i] * n) % p[i];
            s[i] = (s[i] + r[i]) % p[i];
            zeroes[i] += s[i] == 0;
        }
    }
}

int main() {
    int64_t N;
    std::cin >> N;
    std::vector<bool> is_prime(N, true);
    std::vector<int64_t> primes;
    for (int i = 2; i < N; ++i) {
        if (is_prime[i]) {
            primes.push_back(i);
            for (int j = i + i; j < N; j += i) {
                is_prime[j] = false;
            }
        }
    }
    std::cerr << "will check " << primes.size() << " primes, up to " << primes.back() << std::endl;
    auto start = std::chrono::system_clock::now();
    auto last = start;
    constexpr size_t BLOCK_SIZE = 128;
    std::vector<int64_t> cum_sums(primes);
    for (size_t i = 1; i < cum_sums.size(); ++i) {
        cum_sums[i] += cum_sums[i - 1];
    }
#pragma omp parallel
    {
        int64_t num = 0;
        const int thread_num = omp_get_thread_num();
        std::vector<int64_t> r(BLOCK_SIZE), s(BLOCK_SIZE), zeroes(BLOCK_SIZE);
#pragma omp for schedule(static, 1)
        for (size_t block_start = 0; block_start < primes.size(); block_start += BLOCK_SIZE) {
            if (thread_num == 0) {
                auto cur = std::chrono::system_clock::now();
                if (std::chrono::duration<double>(cur - last).count() > 10) {
                    double t = std::chrono::duration<double>(cur - start).count();
                    double expected_total = t / cum_sums[block_start] * cum_sums.back();
#pragma omp critical(Write)
                    std::cerr << BLUE << "i=" << block_start << ", t=" << t << ", expected_total=" << expected_total << RESET << std::endl;
                    last = cur;
                }
            }

            const size_t block_size = std::min(BLOCK_SIZE, primes.size() - block_start);
            process_block(primes.data() + block_start, r.data(), s.data(), zeroes.data(), block_size);
            for (size_t i = 0; i < block_size; ++i) {
                if (zeroes[i] >= 7 || s[i] == 0) {
#pragma omp critical(Write)
                    std::cout << primes[i + block_start] << ' ' << (s[i] == 0) << ' ' << zeroes[i] << std::endl;
                }
            }
        }
    }
    std::cout << "total time " << std::chrono::duration<double>(std::chrono::system_clock::now() - start).count() << std::endl;
    return 0;
}
