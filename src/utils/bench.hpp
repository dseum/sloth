#ifndef BENCH_HPP
#define BENCH_HPP

#include <chrono>
#include <string>

class Bench {
    std::chrono::time_point<std::chrono::steady_clock> start_;

   public:
    enum unit { s, ns };
    Bench();
    double elapsed(unit mode = unit::s);
    std::string format(unit mode = unit::s);
};

#endif
