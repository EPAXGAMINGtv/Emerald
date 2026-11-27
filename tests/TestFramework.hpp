#pragma once

#include <cmath>
#include <functional>
#include <iostream>
#include <string_view>
#include <utility>
#include <vector>

namespace test {

struct Case {
    std::string_view name;
    std::function<void()> run;
};

struct Counters {
    int cases_total = 0;
    int cases_failed = 0;
    int checks_total = 0;
    int checks_failed = 0;
};

inline std::vector<Case>& registry() {
    static std::vector<Case> cases;
    return cases;
}

inline Counters& counters() {
    static Counters values;
    return values;
}

struct Registrar {
    Registrar(std::string_view name, std::function<void()> run) {
        registry().push_back({name, std::move(run)});
    }
};

inline void check(
    bool condition, std::string_view message, const char* file, int line
) {
    Counters& values = counters();
    ++values.checks_total;
    if (!condition) {
        ++values.checks_failed;
        std::cerr << file << ':' << line << ": CHECK failed: " << message
                  << '\n';
    }
}

} // namespace test

#define TEST(name)                                                            \
    static void name();                                                       \
    [[maybe_unused]] static ::test::Registrar name##_registrar{                \
        #name, &name                                                          \
    };                                                                        \
    static void name()

#define CHECK(expr) \
    ::test::check(expr, #expr, __FILE__, __LINE__)

#define CHECK_NEAR(a, b) \
    ::test::check( \
        std::fabs( \
            static_cast<long double>(a) - static_cast<long double>(b) \
        ) <= 1e-6L, \
        #a " ~= " #b, __FILE__, __LINE__ \
    )

