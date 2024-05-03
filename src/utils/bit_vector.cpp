#include "bit_vector.hpp"

#include <cassert>
#include <cstddef>
#include <cstring>

#include "print.hpp"

void BitVector::copy(uint8_t *dest, std::size_t bit_offset) const {
    // index of the next bit to write; also the number of bits written

    uint16_t bits = bits_;
    bits <<= 16 - size();  // left align

    std::size_t bit_left = 8 - bit_offset;

    uint8_t byte_value = 0;
    std::memcpy(&byte_value, dest, 1);
    byte_value = (byte_value | uint16_mask_first[bit_offset]) |
                 static_cast<uint8_t>((bits & uint16_mask_first[bit_left]) >>
                                      (16 - bit_left));
    std::memcpy(dest, &byte_value, 1);

    if (size() <= bit_left) return;
    bits <<= bit_left;
    byte_value = static_cast<uint8_t>((bits & uint16_mask_first[8]) >> 8);
    std::memcpy(dest + 1, &byte_value, 1);

    if (size() <= bit_left + 8) return;
    bits <<= 8;
    byte_value = static_cast<uint8_t>(bits >> 8);
    std::memcpy(dest + 2, &byte_value, 1);
}

void BitVector::set(uint8_t bits) {
    bits_ = bits;
    size_ = 8;
    encoded_ = false;
}

void BitVector::zero(std::size_t size) {
    bits_ = 0;
    size_ = size;
    encoded_ = true;
}

void BitVector::next(std::size_t new_size) {
    assert(new_size <= 16);
    bits_ = (bits_ + 1) << (new_size - size_);
    size_ = new_size;
}

std::size_t BitVector::size() const { return size_; }

uint16_t BitVector::value() const { return bits_; }

std::string BitVector::to_string() const {
    std::string s;
    for (std::size_t i = 0; i < size(); ++i) {
        s += (bits_ & (1 << (size() - i - 1))) ? '1' : '0';
    }
    return s;
}
