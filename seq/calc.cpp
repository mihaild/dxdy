#include <gmp.h>
#include <gmpxx.h>

#include <iostream>
#include <vector>
#include <map>

struct TMember {
    size_t Index = 0;
    int IsPrime = 0;
};

int main(int, const char *argv[]) {
    mpz_class add = std::atoi(argv[1]);
    std::map<mpz_class, TMember> sequence; //TODO: unordered_map
    mpz_class cur = 2;
    size_t primes = 0;
    size_t maxLen = 0;

    while (sequence.find(cur) == sequence.end() && sequence.size() < 1000) {
        int isPrime = mpz_probab_prime_p(cur.get_mpz_t(), 50);
        size_t l = cur.get_str().size(); //TODO: speed up
        maxLen = std::max(l, maxLen);
        if (isPrime == 0) {
            sequence[cur] = TMember{sequence.size(), false};
            cur /= 2;
        } else {
            sequence[cur] = TMember{sequence.size(), true};
            cur = cur * cur + add;
            ++primes;
        }
    }
    std::cout << add << ' ' << cur << ' ' << sequence[cur].Index << ' ' << (sequence.size() - sequence[cur].Index) << ' ' << maxLen << std::endl;
    return 0;
}
