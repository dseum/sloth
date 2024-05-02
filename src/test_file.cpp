#include "test_file.hpp"

#include <sys/mman.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <random>
#include <string>

#include "utils/bench.hpp"
#include "utils/byte_stream.hpp"
#include "utils/print.hpp"

void TestFile::generate(std::string pathname, std::size_t size) {
    OByteStream obs(pathname, size);
    uint8_t *obs_map = obs.map();

    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::poisson_distribution<uint64_t> dis(uint32_t(-1));

    for (std::size_t i = 0; i < obs.size() / 8; ++i) {
        uint64_t val = dis(gen) % 26;
        std::memcpy(obs_map + 8 * i, &val, 8);
    }
    {
        uint64_t val = dis(gen);
        if (obs.size() % 8 != 0) {
            std::memcpy(obs_map + obs.size() / 8, &val, obs.size() % 8);
        }
    }
}
