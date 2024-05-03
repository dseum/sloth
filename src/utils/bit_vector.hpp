#ifndef BIT_VECTOR_HPP
#define BIT_VECTOR_HPP

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <string>

constexpr static uint16_t uint16_mask_last[17] = {
    0b0000000000000000, 0b0000000000000001, 0b0000000000000011,
    0b0000000000000111, 0b0000000000001111, 0b0000000000011111,
    0b0000000000111111, 0b0000000001111111, 0b0000000011111111,
    0b0000000111111111, 0b0000001111111111, 0b0000011111111111,
    0b0000111111111111, 0b0001111111111111, 0b0011111111111111,
    0b0111111111111111, 0b1111111111111111,
};

constexpr static uint16_t uint16_mask_first[17] = {
    0b0000000000000000, 0b1000000000000000, 0b1100000000000000,
    0b1110000000000000, 0b1111000000000000, 0b1111100000000000,
    0b1111110000000000, 0b1111111000000000, 0b1111111100000000,
    0b1111111110000000, 0b1111111111000000, 0b1111111111100000,
    0b1111111111110000, 0b1111111111111000, 0b1111111111111100,
    0b1111111111111110, 0b1111111111111111,
};

class BitVector {
    std::size_t size_ = 0;
    uint16_t bits_ = 0;
    bool encoded_ = false;

   public:
    void set(uint8_t bits);
    void zero(std::size_t size);
    void next(std::size_t new_size);
    void copy(uint8_t *dest, std::size_t bit_offset) const;
    std::size_t size() const;  // number of bits
    uint16_t value() const;
    std::string to_string() const;
};

#endif
