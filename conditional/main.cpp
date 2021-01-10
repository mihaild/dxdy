#include <benchmark/benchmark.h>
#include <vector>
#include <random>

using TInt = uint8_t;
using TVector = std::vector<TInt>;

TVector gen(size_t n, size_t seed) {
    TVector result(n);
    std::mt19937_64 e;
    e.seed(seed);
    std::uniform_int_distribution<TVector::value_type> d(0, std::numeric_limits<TVector::value_type>::max());
    for (auto& x : result) {
        x = d(e);
    }
    return result;
}

TInt with_all(const TVector& values) {
    TInt result = 0;
    for (auto x : values) {
        result += x;
    }
    return result;
}

TInt with_eq(const TVector& values) {
    TInt result = 0;
    for (auto x : values) {
        if ((x & 1) == 0) {
            result += x;
        }
    }
    return result;
}

TInt with_neq(const TVector& values) {
    TInt result = 0;
    for (auto x : values) {
        if ((x & 1) != 0) {
            result += x;
        }
    }
    return result;
}

TInt transform(TInt x) {
    if ((x & 1) == 0) {
        return x;
    }
    return 0;
}

TInt with_nested(const TVector& values) {
    TInt result = 0;
    for (auto x : values) {
        result += transform(x);
    }
    return result;
}

template<auto func>
void BM_test(benchmark::State& state) {
    auto a = gen(state.range(0), 45);
    for (auto _ : state) {
        benchmark::DoNotOptimize(func(a));
    }
}

BENCHMARK_TEMPLATE(BM_test, with_all)->Arg(30000000);
BENCHMARK_TEMPLATE(BM_test, with_neq)->Arg(30000000);
BENCHMARK_TEMPLATE(BM_test, with_eq)->Arg(30000000);
BENCHMARK_TEMPLATE(BM_test, with_nested)->Arg(30000000);

BENCHMARK_MAIN();
