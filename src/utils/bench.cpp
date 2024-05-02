#include "bench.hpp"

#include <chrono>

Bench::Bench() : start_(std::chrono::steady_clock::now()) {}

double Bench::elapsed(unit mode) {
    auto end = std::chrono::steady_clock::now();
    switch (mode) {
        case unit::s: {
            return std::chrono::duration_cast<std::chrono::duration<double>>(
                       end - start_)
                .count();
        }
        case unit::ns: {
            return std::chrono::duration_cast<
                       std::chrono::duration<double, std::nano>>(end - start_)
                .count();
        }
    }
    return 0;
}

std::string Bench::format(unit mode) {
    switch (mode) {
        case unit::s: {
            return std::to_string(elapsed(mode)) + "s";
            break;
        }
        case unit::ns: {
            return std::to_string(elapsed(mode)) + "ns";
        }
    }
    return "";
}
