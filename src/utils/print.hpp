#ifndef PRINT_HPP
#define PRINT_HPP

#include <format>
#include <iostream>
#include <iterator>

static std::ostream_iterator<char> out(std::cout);

template <class... Args>
void print(std::format_string<Args...> fmt, Args&&... args) {
    std::format_to(out, fmt, std::forward<Args>(args)...);
}

template <class... Args>
void println(std::format_string<Args...> fmt, Args&&... args) {
    print(fmt, std::forward<Args>(args)...);
    print("\n");
}

#endif
