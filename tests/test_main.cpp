#include "TestFramework.hpp"

#include <SDL.h>

#include <exception>
#include <iostream>

int main() {
    (void)SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    SDL_SetMainReady();

    test::Counters& values = test::counters();

    for (const test::Case& current : test::registry()) {
        const int checks_before = values.checks_total;
        const int failures_before = values.checks_failed;

        std::cout << "[ RUN ] " << current.name << '\n';
        try {
            current.run();
        } catch (const std::exception& error) {
            std::cerr << "EXCEPTION in " << current.name << ": "
                      << error.what() << '\n';
            std::cerr << "  file: " << __FILE__ << '\n';
            ++values.checks_total;
            ++values.checks_failed;
        }

        ++values.cases_total;
        const int case_failures = values.checks_failed - failures_before;
        if (case_failures == 0) {
            std::cout << "[ PASS ] " << current.name << " ("
                      << values.checks_total - checks_before << " checks)\n";
        } else {
            ++values.cases_failed;
            std::cout << "[ FAIL ] " << current.name << " (" << case_failures
                      << " of " << values.checks_total - checks_before
                      << " checks failed)\n";
        }
    }

    const int checks_passed = values.checks_total - values.checks_failed;
    const int cases_passed = values.cases_total - values.cases_failed;

    std::cout << "\n================ TEST SUMMARY ================\n";
    std::cout << "Cases:  " << cases_passed << " passed, "
              << values.cases_failed << " failed, " << values.cases_total
              << " total\n";
    std::cout << "Checks: " << checks_passed << " passed, "
              << values.checks_failed << " failed, " << values.checks_total
              << " total\n";
    std::cout << "Result: "
              << (values.cases_failed == 0 && values.checks_failed == 0
                      ? "PASS"
                      : "FAIL")
              << '\n';

    return values.cases_failed == 0 && values.checks_failed == 0 ? 0 : 1;
}
