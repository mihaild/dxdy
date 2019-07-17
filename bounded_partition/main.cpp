#include <gmpxx.h>
#include <vector>
#include <chrono>
#include <iostream>
#include <cassert>

using std::vector;
using std::cout;
using std::cin;
using std::pair;
using std::endl;

mpz_class naive_square(int N, int K) {
    vector<vector<mpz_class>> cache(N + 1, vector<mpz_class>(K + 1));
    cache[0][0] = 1;

    for (int n = 0; n <= N; ++n) {
        for (int k = 1; k <= K; ++k) {
            if (k <= n) {
                cache[n][k] = cache[n - k][k] + cache[n][k - 1];
            } else {
                cache[n][k] = cache[n][n];
            }
        }
    }

    return cache[N][K];
}

mpz_class naive_square_no_extra(int N, int K) {
    vector<vector<mpz_class>> cache(N + 1, vector<mpz_class>(K + 1));
    cache[0][0] = 1;

    for (int n = 0; n <= N; ++n) {
        for (int k = 1; k <= K && k <= n; ++k) {
            if (n - k >= k) {
                cache[n][k] = cache[n - k][k] + cache[n][k - 1];
            } else {
                cache[n][k] = cache[n - k][n - k] + cache[n][k - 1];
            }
        }
    }

    return cache[N][K];
}

mpz_class single_line(int N, int K) {
    vector<mpz_class> cache(N + 1);
    cache[0] = 1;
    for (int k = 1; k <= K; ++k) {
        for (int start = 0; start < k; ++start) {
            for (int x = start + k; x <= N; x += k) {
                cache[x] += cache[x - k];
            }
        }
    }

    return cache[N];
}

using TCheckFunction = mpz_class (*)(int, int);
uint64_t TMP = 0;
template<TCheckFunction F> std::pair<double, double> Run(int64_t N, int64_t K, int times=16) {
    assert(F(20, 10) == 530);
    assert(F(1, 1) == 1);
    assert(F(2, 2) == 2);
    assert(F(4, 1) == 1);
    assert(F(4, 2) == 3);
    assert(F(4, 3) == 4);
    assert(F(4, 4) == 5);
    vector<double> t;
    t.reserve(times);
    for (int i = 0; i < times; ++i) {
        auto start = std::chrono::system_clock::now();
        TMP += F(N, K).get_ui();
        t.push_back(std::chrono::duration<double>(std::chrono::system_clock::now() - start).count());
    }
    sort(t.begin(), t.end());
    return std::make_pair(t[times / 4], t[times * 3 / 4]);
}

int main() {
#define CHECK(func)\
    { cout << #func << ' '; auto res = Run<func>(2000, 1000, 16); cout << res.first << ' ' << res.second << endl; }
    CHECK(naive_square);
    CHECK(naive_square_no_extra);
    CHECK(single_line);
    return 0;
}
