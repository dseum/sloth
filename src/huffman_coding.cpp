#include "huffman_coding.hpp"

#include <unistd.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <stack>
#include <vector>

#include "utils/bench.hpp"
#include "utils/byte_stream.hpp"
#include "utils/print.hpp"
#include "utils/sizes.hpp"

namespace HuffmanCoding {
/**
 * Symbol
 */

Symbols::PackageMergeItem::PackageMergeItem() : weight_(0) {}

Symbols::PackageMergeItem::PackageMergeItem(Symbol *symbol)
    : weight_(symbol->weight_) {
    symbols_.push_back(symbol);
}

bool Symbols::PackageMergeItem::operator<(const PackageMergeItem &rhs) const {
    return weight_ < rhs.weight_;
}

Symbols::PackageMergeItem Symbols::PackageMergeItem::operator+(
    const PackageMergeItem &rhs) const {
    PackageMergeItem item;
    item.symbols_.reserve(symbols_.size() + rhs.symbols_.size());
    item.symbols_.insert(item.symbols_.end(), symbols_.begin(), symbols_.end());
    item.symbols_.insert(item.symbols_.end(), rhs.symbols_.begin(),
                         rhs.symbols_.end());
    item.weight_ = weight_ + rhs.weight_;
    return item;
}

Symbols::Symbols() : symbols_(nullptr), size_(0) {}

Symbols::~Symbols() { delete[] symbols_; }

Symbol &Symbols::operator[](std::size_t index) { return symbols_[index]; }

void Symbols::initialize(std::size_t size) {
    assert(symbols_ == nullptr);
    symbols_ = new Symbol[size];
    size_ = size;
}

void Symbols::fill_lengths() {
    if (size_ == 1) {
        return;
    }
    std::size_t cutoff = 2 * size_ - 2;
    std::vector<PackageMergeItem> items;
    items.reserve(size_);
    for (std::size_t i = 0; i < size_; ++i) {
        items.emplace_back(&symbols_[i]);
    }
    while (items.size() < cutoff) {
        std::sort(items.begin(), items.end());
        std::vector<PackageMergeItem> packaged_items;
        packaged_items.reserve(items.size() / 2);
        std::size_t i;
        for (i = 0; i < items.size() - 2; i += 2) {
            packaged_items.push_back(std::move(items[i] + items[i + 1]));
        }
        if ((items.size() & 1) == 0) {
            packaged_items.push_back(std::move(items[i] + items[i + 1]));
        }
        items.swap(packaged_items);
        for (std::size_t i = 0; i < size_; ++i) {
            items.emplace_back(&symbols_[i]);
        }
    }
    std::sort(items.begin(), items.end());
    for (std::size_t i = 0; i < cutoff; ++i) {
        for (Symbol *&symbol : items[i].symbols_) {
            ++symbol->length_;
        }
    }
}

BitVector *Symbols::generate_codes() {
    BitVector *codes = new BitVector[256];

    if (size_ == 0) {
        codes[symbols_[0].value_].zero(1);
        return codes;
    }

    BitVector code;
    std::sort(symbols_, symbols_ + size_,
              [](const Symbol &lhs, const Symbol &rhs) {
                  if (lhs.length_ == rhs.length_) {
                      return lhs.value_ < rhs.value_;
                  }
                  return lhs.length_ < rhs.length_;
              });
    code.zero(symbols_[0].length_);
    std::size_t i;
    for (i = 0; i < size_ - 1; ++i) {
        codes[symbols_[i].value_] = code;
        std::size_t length = symbols_[i + 1].length_;
        assert(length > 0 && length <= 16);
        code.next(length);
    }
    codes[symbols_[size_ - 1].value_] = code;
    return codes;
}

Symbol *Symbols::data() { return symbols_; }

std::size_t Symbols::size() const { return size_; }

/**
 * Processor
 */

void Serial::Processor::encode(std::string pathname,
                               std::string encoded_pathname) {
    IByteStream ibs(pathname);

    Symbols symbols;
    {
        std::array<std::size_t, 256> counts = {0};  // assuming only ASCII
        for (std::size_t i = 0; i < ibs.size(); ++i) {
            ++counts[ibs[i]];
        }
        symbols.initialize(256 - std::count(counts.begin(), counts.end(), 0));
        for (std::size_t i = 0, end = 0; i < 256; ++i) {
            if (counts[i] != 0) {
                std::size_t j = end++;
                symbols[j] = {
                    .weight_ = counts[i],
                    .value_ = static_cast<uint8_t>(i),
                    .length_ = 0,
                };
            }
        }
    }

    symbols.fill_lengths();
    BitVector *codes = symbols.generate_codes();

    // write
    {
        std::size_t encoded_size;
        {
            std::size_t bits_used = header_bits;
            for (std::size_t i = 0; i < symbols.size(); ++i) {
                bits_used += symbols[i].weight_ * symbols[i].length_;
            }
            encoded_size = (bits_used + 7) / 8;
        }
        if (encoded_size > ibs.size()) {
            println(
                "sloth: compressed file will be larger than original "
                "hahaha");
        }

        OByteStream obs(encoded_pathname, encoded_size);
        uint8_t *obs_map = obs.map();

        // header
        {
            /*
             * 0-7: old size
             * 8-263: code lengths
             */
            std::size_t old_size = ibs.size();
            std::memcpy(obs_map, &old_size, 8);

            // trie using DEFLATE spec
            // https://www.ietf.org/rfc/rfc1951.txt
            for (std::size_t i = 0; i < 256; ++i) {
                uint8_t code_size =
                    codes[i].size();  // max code size is 255 since ASCII
                std::memcpy(obs_map + i + 8, &code_size, 1);
            }
        }

        // body
        std::size_t bits_used = header_bits;
        for (std::size_t i = 0; i < ibs.size(); ++i) {
            uint8_t value = ibs[i];
            BitVector &code = codes[value];
            code.copy(obs_map + bits_used / 8, bits_used % 8);
            bits_used += code.size();
        }
        assert(encoded_size == (bits_used + 7) / 8);
        println("sloth: Compression ratio: {:.2f}",
                ibs.size() / static_cast<double>(encoded_size));
    }
}

void Serial::Processor::decode(std::string encoded_pathname,
                               std::string decoded_pathname) {
    IByteStream ibs(encoded_pathname);
    const uint8_t *ibs_map = ibs.map();
    std::size_t encoded_size = ibs.size();

    std::size_t decoded_size = 0;
    std::memcpy(&decoded_size, ibs_map, 8);

    std::array<uint8_t, 256> code_lengths;
    std::memcpy(code_lengths.data(), ibs_map + 8, 256);
    {
        std::pair<uint8_t, std::size_t> barriers[256] = {{0, 0}};
        Symbols symbols;
        symbols.initialize(
            256 - std::count(code_lengths.begin(), code_lengths.end(), 0));
        for (std::size_t i = 0, symbols_i = 0; i < 256; ++i) {
            if (code_lengths[i] != 0) {
                symbols[symbols_i++] = {
                    .weight_ = 0,
                    .value_ = static_cast<uint8_t>(i),
                    .length_ = code_lengths[i],
                };
            }
        }
        BitVector *codes = symbols.generate_codes();
        for (std::size_t i = 0; i < symbols.size() - 1; ++i) {
            uint8_t value_a = symbols[i].value_;
            BitVector &code_a = codes[value_a];
            uint8_t barrier_a = static_cast<uint8_t>(code_a.value())
                                << (8 - code_a.size());

            uint8_t value_b = symbols[i + 1].value_;
            BitVector &code_b = codes[value_b];
            uint8_t barrier_b = static_cast<uint8_t>(code_b.value())
                                << (8 - code_b.size());

            for (std::size_t j = barrier_a; j < barrier_b; ++j) {
                barriers[j] = {value_a, code_a.size()};
            }
        }
        {
            Symbol &symbol = symbols[symbols.size() - 1];
            BitVector &code = codes[symbol.value_];
            uint8_t barrier =
                (static_cast<uint8_t>(code.value()) << (8 - code.size()));
            for (std::size_t i = barrier; i < 256; ++i) {
                barriers[i] = {symbol.value_, code.size()};
            }
        }

        OByteStream obs(decoded_pathname + ".res", decoded_size);
        uint8_t *obs_map = obs.map();

        std::size_t bits_read = header_bits;
        for (size_t i = 0; i < decoded_size; ++i) {
            std::size_t byte_offset = bits_read / 8;
            uint8_t byte_1 = ibs[byte_offset];
            uint8_t byte_2 = 0;
            if (byte_offset - 1 < encoded_size) {
                byte_2 = ibs[byte_offset + 1];
            }

            std::size_t bit_offset = bits_read % 8;
            byte_1 <<= bit_offset;
            byte_2 >>= (8 - bit_offset);
            uint8_t byte = byte_1 | byte_2;

            std::pair<uint8_t, std::size_t> barrier = barriers[byte];
            std::memcpy(obs_map + i, &(barrier.first), 1);
            bits_read += barrier.second;
        }
    }
}

void Parallel::Processor::encode(std::string pathname,
                                 std::string encoded_pathname) {
    IByteStream ibs(pathname);

    Symbols symbols;
    {
        std::array<std::size_t, 256> counts = {0};  // assuming only ASCII
#pragma omp parallel
        {
            std::array<std::size_t, 256> _counts = {0};
#pragma omp for nowait schedule(static, Sizes::page)
            for (std::size_t i = 0; i < ibs.size(); ++i) {
                ++_counts[ibs[i]];
            }
#pragma omp critical
            {
                for (std::size_t i = 0; i < 256; ++i) {
                    counts[i] += _counts[i];
                }
            }
        }
        symbols.initialize(256 - std::count(counts.begin(), counts.end(), 0));
        for (std::size_t i = 0, end = 0; i < 256; ++i) {
            if (counts[i] != 0) {
                std::size_t j = end++;
                symbols[j] = {
                    .weight_ = counts[i],
                    .value_ = static_cast<uint8_t>(i),
                    .length_ = 0,
                };
            }
        }
    }

    symbols.fill_lengths();
    BitVector *codes = symbols.generate_codes();

    // write
    {
        std::size_t encoded_size;
        {
            std::size_t bits_used = header_bits;
            for (std::size_t i = 0; i < symbols.size(); ++i) {
                bits_used += symbols[i].weight_ * symbols[i].length_;
            }
            encoded_size = (bits_used + 7) / 8;
        }
        if (encoded_size > ibs.size()) {
            println(
                "sloth: compressed file will be larger than original "
                "hahaha");
        }

        OByteStream obs(encoded_pathname, encoded_size);
        uint8_t *obs_map = obs.map();

        // header
        {
            /*
             * 0-7: old size
             * 8-263: code lengths
             */
            std::size_t old_size = ibs.size();
            std::memcpy(obs_map, &old_size, 8);

            // trie using DEFLATE spec
            // https://www.ietf.org/rfc/rfc1951.txt
            for (std::size_t i = 0; i < 256; ++i) {
                uint8_t code_size =
                    codes[i].size();  // max code size is 255 since ASCII
                std::memcpy(obs_map + i + 8, &code_size, 1);
            }
        }

        // body
        if (ibs.size() > Sizes::page * 4) {
            std::size_t intervals_size =
                (ibs.size() + Sizes::page - 1) / Sizes::page;
            std::tuple<std::size_t, std::size_t, std::size_t> *intervals =
                new std::tuple<std::size_t, std::size_t,
                               std::size_t>[intervals_size];
#pragma omp parallel for schedule(static)
            for (std::size_t i = 0; i < ibs.size(); i += Sizes::page) {
                std::size_t counts[256] = {0};
                for (std::size_t j = i; j < i + Sizes::page && j < ibs.size();
                     ++j) {
                    uint8_t value = ibs[j];
                    ++counts[value];
                }
                std::size_t interval_bits_used = 0;
#pragma omp parallel for reduction(+ : interval_bits_used)
                for (std::size_t j = 0; j < 256; ++j) {
                    interval_bits_used += counts[j] * codes[j].size();
                }
                intervals[i / Sizes::page] = {i, 0, interval_bits_used};
            }
            for (std::size_t i = 1; i < intervals_size; ++i) {
                std::get<1>(intervals[i]) += std::get<2>(intervals[i - 1]);
                std::get<2>(intervals[i]) += std::get<2>(intervals[i - 1]);
            }

#pragma omp parallel for schedule(dynamic)
            for (std::size_t i = 0; i < intervals_size; ++i) {
                std::tuple<std::size_t, std::size_t, std::size_t> &interval =
                    intervals[i];
                std::size_t ibs_i = std::get<0>(interval);
                std::size_t bits_used = header_bits + std::get<1>(interval);
                for (std::size_t j = ibs_i;
                     j < ibs_i + Sizes::page && j < ibs.size(); ++j) {
                    uint8_t value = ibs[j];
                    BitVector &code = codes[value];
                    code.copy(obs_map + bits_used / 8, bits_used % 8);
                    bits_used += code.size();
                }
            }
            delete[] intervals;
        } else {
            std::size_t bits_used = header_bits;
            for (std::size_t i = 0; i < ibs.size(); ++i) {
                uint8_t value = ibs[i];
                BitVector &code = codes[value];
                code.copy(obs_map + bits_used / 8, bits_used % 8);
                bits_used += code.size();
            }
        }
        println("sloth: Compression ratio: {:.2f}",
                ibs.size() / static_cast<double>(encoded_size));
    }
}

void Parallel::Processor::decode(std::string encoded_pathname,
                                 std::string decoded_pathname) {
    Bench bench;
    IByteStream ibs(encoded_pathname);
    const uint8_t *ibs_map = ibs.map();
    size_t encoded_size = ibs.size();

    std::size_t decoded_size = 0;
    std::memcpy(&decoded_size, ibs_map, 8);

    std::array<uint8_t, 256> code_lengths;
    std::memcpy(code_lengths.data(), ibs_map + 8, 256);
    {
        std::pair<uint8_t, std::size_t> barriers[256] = {{0, 0}};
        Symbols symbols;
        symbols.initialize(
            256 - std::count(code_lengths.begin(), code_lengths.end(), 0));
        for (std::size_t i = 0, symbols_i = 0; i < 256; ++i) {
            if (code_lengths[i] != 0) {
                symbols[symbols_i++] = {
                    .weight_ = 0,
                    .value_ = static_cast<uint8_t>(i),
                    .length_ = code_lengths[i],
                };
            }
        }
        BitVector *codes = symbols.generate_codes();
        for (std::size_t i = 0; i < symbols.size() - 1; ++i) {
            uint8_t value_a = symbols[i].value_;
            BitVector &code_a = codes[value_a];
            uint8_t barrier_a = static_cast<uint8_t>(code_a.value())
                                << (8 - code_a.size());

            uint8_t value_b = symbols[i + 1].value_;
            BitVector &code_b = codes[value_b];
            uint8_t barrier_b = static_cast<uint8_t>(code_b.value())
                                << (8 - code_b.size());

            for (std::size_t j = barrier_a; j < barrier_b; ++j) {
                barriers[j] = {value_a, code_a.size()};
            }
        }
        {
            Symbol &symbol = symbols[symbols.size() - 1];
            BitVector &code = codes[symbol.value_];
            uint8_t barrier =
                (static_cast<uint8_t>(code.value()) << (8 - code.size()));
            for (std::size_t i = barrier; i < 256; ++i) {
                barriers[i] = {symbol.value_, code.size()};
            }
        }
        println("Header/Elapsed {}", bench.format());

        if (decoded_size > Sizes::page * 4) {
            Bench bench1;
            std::size_t intervals_size =
                (decoded_size + Sizes::page - 1) / Sizes::page;
            std::tuple<size_t, size_t> *intervals =
                new std::tuple<size_t, size_t>[intervals_size];

            {
                std::size_t bits_read = header_bits;
                for (std::size_t i = 0; i < decoded_size; i += Sizes::page) {
                    intervals[i / Sizes::page] = {i, bits_read};
                    for (std::size_t j = i;
                         j < i + Sizes::page && j < decoded_size; ++j) {
                        std::size_t byte_offset = bits_read / 8;

                        uint8_t byte_1 = ibs[byte_offset];
                        uint8_t byte_2 = 0;
                        if (byte_offset - 1 < encoded_size) {
                            byte_2 = ibs[byte_offset + 1];
                        }

                        std::size_t bit_offset = bits_read % 8;
                        byte_1 <<= bit_offset;
                        byte_2 >>= (8 - bit_offset);
                        uint8_t byte = byte_1 | byte_2;

                        std::pair<uint8_t, std::size_t> barrier =
                            barriers[byte];
                        bits_read += barrier.second;
                    }
                }
            }
            println("Preprocess/Elapsed {}", bench1.format());

            OByteStream obs(decoded_pathname + ".res", decoded_size);
            uint8_t *obs_map = obs.map();

#pragma omp parallel for schedule(static)
            for (std::size_t i = 0; i < intervals_size; ++i) {
                std::tuple<size_t, size_t> &interval = intervals[i];
                std::size_t obs_i = std::get<0>(interval);
                std::size_t bits_read = std::get<1>(interval);
                for (std::size_t j = obs_i;
                     j < obs_i + Sizes::page && j < ibs.size(); ++j) {
                    std::size_t byte_offset = bits_read / 8;
                    uint8_t byte_1 = ibs[byte_offset];
                    uint8_t byte_2 = 0;
                    if (byte_offset - 1 < encoded_size) {
                        byte_2 = ibs[byte_offset + 1];
                    }

                    std::size_t bit_offset = bits_read % 8;
                    byte_1 <<= bit_offset;
                    byte_2 >>= (8 - bit_offset);
                    uint8_t byte = byte_1 | byte_2;

                    std::pair<uint8_t, std::size_t> barrier = barriers[byte];
                    std::memcpy(obs_map + j, &(barrier.first), 1);
                    bits_read += barrier.second;
                }
            }
            delete[] intervals;
        } else {
            OByteStream obs(decoded_pathname + ".res", decoded_size);
            uint8_t *obs_map = obs.map();

            std::size_t bits_read = header_bits;
            for (size_t i = 0; i < decoded_size; ++i) {
                std::size_t byte_offset = bits_read / 8;
                uint8_t byte_1 = ibs[byte_offset];
                uint8_t byte_2 = 0;
                if (byte_offset - 1 < encoded_size) {
                    byte_2 = ibs[byte_offset + 1];
                }

                std::size_t bit_offset = bits_read % 8;
                byte_1 <<= bit_offset;
                byte_2 >>= (8 - bit_offset);
                uint8_t byte = byte_1 | byte_2;

                std::pair<uint8_t, std::size_t> barrier = barriers[byte];
                std::memcpy(obs_map + i, &(barrier.first), 1);
                bits_read += barrier.second;
            }
        }
    }
}

}  // namespace HuffmanCoding
