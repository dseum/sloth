#include "bit_vector.hpp"

#include <cassert>
#include <cstddef>
#include <cstring>

#include "print.hpp"

void BitVector::copy(uint8_t *dest, std::size_t bit_offset) const {
    assert(size() < 16);
    uint8_t byte_val = 0;
    std::memcpy(&byte_val, dest, 1);
    std::size_t bit_write_size = 8 - bit_offset;
    byte_val =
        (byte_val & mask_first[bit_offset]) |
        uint8_t((bits_ & mask_first[bit_write_size]) >> (16 - bit_write_size));
    std::memcpy(dest, &byte_val, 1);

    if (size() <= bit_write_size) return;
    byte_val =
        uint8_t((bits_ & mask_middle[bit_write_size]) >> (8 - bit_write_size));
    std::memcpy(dest + 1, &byte_val, 1);

    if (size() <= bit_write_size + 8) return;
    byte_val = uint8_t((bits_ & mask_last[8 - bit_write_size])
                       << (8 + bit_write_size));
    std::memcpy(dest + 2, &byte_val, 1);
}

void BitVector::zero() {
    bits_ = 0;
    size_ = 1;
}

void BitVector::next(std::size_t new_size) {
    bits_ = (bits_ + 1) << (new_size - size_);
    size_ = new_size;
}

std::size_t BitVector::size() const { return size_; }

uint16_t BitVector::bits() const { return bits_; }
