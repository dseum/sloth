#ifndef HUFFMAN_CODING_HPP
#define HUFFMAN_CODING_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "utils/bit_vector.hpp"

namespace HuffmanCoding {

struct Symbol {
    uint64_t weight_;
    uint8_t value_;
    uint8_t length_;
};

class Symbols {
    struct PackageMergeItem {
        std::vector<Symbol *> symbols_;
        std::size_t weight_;

        PackageMergeItem();
        PackageMergeItem(Symbol *symbol);
        bool operator<(const PackageMergeItem &rhs) const;
        PackageMergeItem operator+(const PackageMergeItem &rhs) const;
    };

    Symbol *symbols_;
    std::size_t size_;

   public:
    Symbols();
    ~Symbols();
    Symbol *data();
    Symbol &operator[](std::size_t index);
    void initialize(std::size_t size);
    std::size_t size() const;
    void fill_lengths();
    BitVector *generate_codes();
};

constexpr std::size_t header_bits = (8 + 256) * 8;

namespace Serial {
class Processor {
   public:
    static void encode(std::string pathname, std::string encoded_pathname);
    static void decode(std::string encoded_pathname, std::string pathname);
};
}  // namespace Serial

namespace Parallel {
class Processor {
   public:
    static void encode(std::string pathname, std::string encoded_pathname);
    static void decode(std::string encoded_pathname, std::string pathname);
};
}  // namespace Parallel
}  // namespace HuffmanCoding

#endif
