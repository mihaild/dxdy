#include <iostream>
#include <vector>
#include <unordered_set>
#include <iterator>
#include <cmath>
#include <chrono>
#include <array>

const int64_t MAX_ROOT = 1000000;

std::vector<int64_t> CubesVector;
std::unordered_set<int64_t> CubesHash;

bool CheckVector(int64_t x) {
    return *std::lower_bound(CubesVector.begin(), CubesVector.end(), x) == x;
}

bool CheckHash(int64_t x) {
    return CubesHash.count(x);
}

struct TCubeIntInterator: public std::iterator<std::random_access_iterator_tag, int64_t, int64_t, int64_t*, int64_t> {
    int64_t Value;
    TCubeIntInterator(int64_t value): Value(value) {
    }

    int64_t operator*() const {
        return Value * Value * Value;
    }

    void operator++() {
        ++Value;
    }

    void operator--() {
        --Value;
    }

    void operator+=(int64_t x) {
        Value += x;
    }

    int64_t operator-(const TCubeIntInterator& other) const {
        return Value - other.Value;
    }
};

bool CheckBinary(int64_t x) {
    return *std::lower_bound(TCubeIntInterator(0), TCubeIntInterator(MAX_ROOT), x) == x;
}

inline bool CheckFromApprox(int64_t x, int64_t r) {
    while (r * r * r < x) {
        ++r;
    }
    while (r * r * r > x) {
        --r;
    }
    return r * r * r == x;
}

bool CheckPow(int64_t x) {
    int64_t r = pow(x, 1.0 / 3);
    return CheckFromApprox(x, r);
}

bool CheckIcbrt(int64_t x) {
    int64_t y;
    int64_t b;
    int64_t _x = x;

    y = 0;
    for (int s = 63; s >= 0; s -= 3) {
        y += y;
        b = 3 * y * (y + 1) + 1;
        if ((x >> s) >= b) {
            x -= b << s;
            y++;
        }
    }
    return y * y * y == _x;
}

/*bool CheckNewton(int64_t x) {
    int64_t r = MAX_ROOT / 10;
    for (int i = 0; i < 9; ++i) {
        r = r - (r * r * r - x) / (3 * r * r);
    }
    return (r * r * r == x) ||  CheckFromApprox(x, r);
}*/

using TCheckFunction = bool (*)(int64_t);

int64_t TotalR = 0;
template<TCheckFunction F> double GetTime(int64_t left, int64_t right) {
    auto start = std::chrono::system_clock::now();
    int r = 0;
    for (int64_t x = left; x < right; ++x) {
        if (F(x)) {
            ++r;
        }
    }
    TotalR += r;
    return std::chrono::duration<double>(std::chrono::system_clock::now() - start).count();
}

bool CheckFunc(TCheckFunction F) {
    for (int64_t i = 0; i < 10000; ++i) {
        if (F(i) != CheckVector(i)) {
            return false;
        }
    }
    for (int64_t i = 1000000; i < 1100000; ++i) {
        if (F(i) != CheckVector(i)) {
            return false;
        }
    }
    for (int64_t i = 10000; i < 11000; ++i) {
        if (!F(i * i * i) || F(i * i * i + 1) || F(i * i * i - 1)) {
            return false;
        }
    }
    return true;
}

template<int64_t MOD> std::array<bool, MOD> ModulesBool;
template<int64_t MOD> inline bool PrecheckModulo(int64_t x) {
    return ModulesBool<MOD>[x % MOD];
}

template<TCheckFunction PreCheck, TCheckFunction F> bool PrecheckedCheck(int64_t x) {
    return PreCheck(x) && F(x);
}

inline bool PrechekTrivial(int64_t) {
    return true;
}

int main() {
    {
#define INIT_MOD(m)\
        do {\
            std::fill(ModulesBool<m>.begin(), ModulesBool<m>.end(), false);\
            for (int64_t i = 0; i < m; ++i) {\
                ModulesBool<m>[(i * i * i) % m] = true;\
            }\
        } while(false);

        INIT_MOD(819);
        INIT_MOD(88236);
        INIT_MOD(575757);
#undef INIT_MOD
    }

    CubesVector.reserve(MAX_ROOT);
    CubesHash.reserve(MAX_ROOT);

    for (int64_t i = 0; i < MAX_ROOT; ++i) {
        CubesVector.push_back(i * i * i);
        CubesHash.insert(i * i * i);
    }

    {
        for (auto func : {CheckVector, CheckHash, CheckBinary, CheckPow, CheckIcbrt, PrecheckedCheck<PrecheckModulo<819>, CheckVector>}) {
            if (!CheckFunc(func)) {
                std::cerr << "error..." << std::endl;
                std::terminate();
            }
        }
    }

    int64_t left = 1000000000;
    int64_t right = left * 2;

#define OUT_TIME(Precheck, Check)\
    std::cout << "    " << #Check << ' ' << GetTime<PrecheckedCheck<Precheck, Check>>(left, right) << std::endl;

#define OUT_TIME_ALL(Precheck)\
    std::cout << #Precheck << std::endl;\
    OUT_TIME(Precheck, CheckVector);\
    OUT_TIME(Precheck, CheckHash);\
    OUT_TIME(Precheck, CheckBinary);\
    OUT_TIME(Precheck, CheckPow);\
    OUT_TIME(Precheck, CheckIcbrt);\
    std::cout << std::endl;

    OUT_TIME_ALL(PrechekTrivial);
    OUT_TIME_ALL(PrecheckModulo<819>);
    OUT_TIME_ALL(PrecheckModulo<88236>);
    OUT_TIME_ALL(PrecheckModulo<575757>);
    OUT_TIME_ALL((PrecheckedCheck<PrecheckModulo<88236>, PrecheckModulo<575757>>));
    OUT_TIME_ALL((PrecheckedCheck<PrecheckModulo<575757>, PrecheckModulo<88236>>));


    return 0;
}
