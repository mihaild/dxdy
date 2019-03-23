

#include <gmp.h>
#include <gmpxx.h>

#include <iostream>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>

struct TMember {
    size_t Index = 0;
    bool IsPrime = 0;
};

int main(int, const char* argv[]) {
    mpz_class add = std::atoi(argv[1]);
    std::map<mpz_class, TMember> sequence; //TODO: unordered_map
    mpz_class cur = 2;
    size_t primes = 0;
    size_t maxLen = 0;

    const size_t THREADS_NUMBER = 2;
    auto start = std::chrono::steady_clock::now();
    while (sequence.find(cur) == sequence.end()) {
        ++primes;
        std::cout << cur << ' ' << sequence.size() << std::endl;
        sequence[cur] = TMember{sequence.size(), true};
        cur = cur * cur + add;
        size_t l = cur.get_str().size(); //TODO: speed up
        maxLen = std::max(l, maxLen);
        size_t shiftNeed = 0;
        std::cerr << sequence.size() << ' ' << l << ' ' << maxLen << ' ' << primes << ' ' << std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::steady_clock::now() - start).count() << std::endl;
        if (l > 1000) {
            std::vector<std::thread> threads;
            std::atomic<size_t> shiftCheck = 0;
            std::atomic<size_t> shiftFound = -1;
            for (size_t i = 0; i < THREADS_NUMBER; ++i) {
                auto threadFunc = [&cur, &shiftCheck, &shiftFound]() {
                    while (true) {
                        size_t shift = shiftCheck++;
                        if (shift > shiftFound) {
                            return;
                        }
                        mpz_class number = (cur >> shift);
                        if (mpz_probab_prime_p(number.get_mpz_t(), 50)) {
                            size_t prev = shiftFound;
                            while (prev > shift && !shiftFound.compare_exchange_weak(prev, shift));
                            return;
                        }
                    }
                };
                threads.emplace_back(threadFunc);
            }
            for (auto& thread : threads) {
                thread.join();
            }
            shiftNeed = shiftFound;
        } else {
            mpz_class toCheck = cur;
            while (!mpz_probab_prime_p(toCheck.get_mpz_t(), 50)) {
                toCheck >>= 1;
                ++shiftNeed;
            }
        }
        for (size_t i = 0; i < shiftNeed; ++i) {
            if (sequence.find(cur) != sequence.end()) {
                break;
            }
            sequence[cur] = TMember{sequence.size(), false};
            cur >>= 1;
        }
    }
    std::cout << add << ' ' << cur << ' ' << sequence[cur].Index << ' ' << (sequence.size() - sequence[cur].Index) << ' ' << maxLen << std::endl;
    return 0;
}
