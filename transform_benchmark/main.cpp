#include <chrono>
#include <vector>
#include <algorithm>
#include <functional>
#include <numeric>
#include <iostream>

using TVector = std::vector<int>;

void AddCycle(const TVector& a, const TVector& b, TVector& c) {
    for (size_t i = 0; i < a.size(); ++i) {
        c[i] = a[i] + b[i];
    }
}

void AddTransform(const TVector& a, const TVector& b, TVector& c) {
    std::transform(a.cbegin(), a.cend(), b.cbegin(), c.begin(), std::plus<int>());
}

using TF = void(*)(const TVector&, const TVector&, TVector&);
template<TF F> double GetSingleTime(size_t N) {
    TVector a(N, 0), b(N, 0), c(N);
    std::iota(a.begin(), a.end(), 0);
    std::iota(b.begin(), b.end(), N);
    auto begin = std::chrono::system_clock::now();
    F(a, b, c);
    auto end = std::chrono::system_clock::now();
    return std::chrono::duration<double>(end - begin).count() * 1e6;
}

template<TF F> std::pair<double, double> GetMultiTime(size_t N) {
    std::vector<double> times;
    for (size_t i = 0; i < 16; ++i) {
        times.push_back(GetSingleTime<F>(N));
    }
    times.clear();
    for (size_t i = 0; i < 16; ++i) {
        times.push_back(GetSingleTime<F>(N));
    }
    std::sort(times.begin(), times.end());
    return {times[4], times[8]};
}

int main() {
    for (int i = 0; i <= 25; i += 5) {
        std::cout << i << " | ";
        std::pair<double, double> tr, c;
        if (i % 2 == 0) {
            tr = GetMultiTime<AddTransform>(1 << i);
            c = GetMultiTime<AddCycle>(1 << i);
        } else {
            c = GetMultiTime<AddCycle>(1 << i);
            tr = GetMultiTime<AddTransform>(1 << i);
        }
        std::cout << tr.first << " | " << tr.second << " | ";
        std::cout << c.first << " | " << c.second << std::endl;
    }
    return 0;
}
