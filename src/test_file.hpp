#ifndef TEST_FILE_HPP
#define TEST_FILE_HPP

#include <cstddef>
#include <string>

#include "utils/byte_stream.hpp"

class TestFile {
   public:
    static void generate(std::string pathname, std::size_t size);
};

#endif
