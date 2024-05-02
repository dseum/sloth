#include "huffman_coding.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <stack>
#include <vector>

#include "utils/byte_stream.hpp"
#include "utils/print.hpp"

namespace HuffmanCoding {
/**
 * Package
 */

Package::Package() : weight_(0), value_(0) {}

Package::Package(uint8_t value, std::size_t weight, Package *self)
    : weight_(weight), value_(value) {
    within_.push_back(self);
}

bool Package::operator<(const Package &rhs) const {
    return weight_ < rhs.weight_;
}

void Package::add(Package &rhs) {
    within_.insert(within_.end(), rhs.within_.begin(), rhs.within_.end());
    weight_ += rhs.weight_;
}

/**
 * Node
 */

Node::Node() : left_(nullptr), right_(nullptr), value_(0) {}

Node::Node(uint8_t value, std::size_t weight, Node *left, Node *right)
    : left_(left), right_(right), value_(value) {}

/**
 * Processor
 */

void Serial::Processor::encode(std::string pathname,
                               std::string encoded_pathname) {
    IByteStream ibs(pathname);

    PackageArray pkgs;
    {
        std::array<std::size_t, 256> counts;  // assuming only ASCII
        counts.fill(0);

        for (std::size_t i = 0; i < ibs.size(); ++i) {
            ++counts[ibs[i]];
        }

        pkgs.second = 256 - std::count(counts.begin(), counts.end(), 0);
        pkgs.first = new Package[pkgs.second];
        for (std::size_t i = 0, end = 0; i < 256; ++i) {
            if (counts[i] != 0) {
                print("{}: {}; ", i, counts[i]);
                std::size_t j = end++;
                pkgs.first[j] = Package(i, counts[i], &pkgs.first[j]);
            }
        }
        print("\n");
    }

    // makes codes
    std::array<BitVector, 256> codes;
    {
        std::size_t min_pkgs = 2 * pkgs.second - 2;
        if (min_pkgs == 0) {
            codes[pkgs.first[0].value_].zero();
        } else {
            std::vector<Package> viewer(pkgs.first, pkgs.first + pkgs.second);
            while (viewer.size() < min_pkgs) {
                std::sort(viewer.begin(), viewer.end());
                std::size_t i;
                std::vector<Package> new_viewer;
                new_viewer.reserve(viewer.size() / 2);
                for (i = 0; i < viewer.size() - 2; i += 2) {
                    viewer[i].add(viewer[i + 1]);
                    new_viewer.push_back(viewer[i]);
                }
                if ((viewer.size() & 1) == 0) {
                    viewer[i].add(viewer[i + 1]);
                    new_viewer.push_back(viewer[i]);
                }
                viewer.swap(new_viewer);
                viewer.insert(viewer.end(), pkgs.first,
                              pkgs.first + pkgs.second);
            }
            std::sort(viewer.begin(), viewer.end());
            for (Package &pkg : viewer) {
                for (Package *&value : pkg.within_) {
                    ++value->length_;
                }
            }
            std::sort(pkgs.first, pkgs.first + pkgs.second,
                      [](const Package &lhs, const Package &rhs) {
                          if (lhs.length_ == rhs.length_) {
                              return lhs.value_ < rhs.value_;
                          }
                          return lhs.length_ < rhs.length_;
                      });
            // always should be some value with length = 0
            BitVector code;
            code.zero();
            for (std::size_t i = 0; i < min_pkgs - 1; ++i) {
                codes[pkgs.first[i].value_] = code;
                code.next(pkgs.first[i + 1].length_);
            }
            codes[pkgs.first[min_pkgs - 1].value_] = code;
        }
        delete[] pkgs.first;
    }

    // output
    {
        std::size_t max_size = ibs.size();
        OByteStream obs(encoded_pathname, max_size);
        uint8_t *obs_map = obs.map();

        // header
        {
            /*
             * 0-7: original size
             * 8: padding
             * 9-264: code sizes
             */
            std::memcpy(obs_map, &max_size, 8);

            // trie using DEFLATE spec
            // https://www.ietf.org/rfc/rfc1951.txt
            for (std::size_t i = 0; i < 256; ++i) {
                uint8_t code_size =
                    codes[i].size();  // max code size is 255 since ASCII
                std::memcpy(obs_map + i + 8 + 1, &code_size, 1);
            }
        }

        // body
        std::size_t bits_used = 8 + 1 + 256;
        for (std::size_t i = 0; i < ibs.size(); ++i) {
            uint8_t val = ibs[i];
            BitVector &code = codes[val];
            std::size_t bytes_i = bits_used / 8;
            std::size_t new_size = (bits_used + code.size() + 7) / 8;
            if (new_size > max_size) {
                max_size = std::max(max_size * 2, new_size);
                obs_map = obs.truncate(max_size *= 2);
            }
            code.copy(obs_map + bytes_i, bits_used % 8);
            bits_used += code.size();
        }
        uint8_t padding = (8 - bits_used % 8) % 8;
        std::memcpy(obs_map, &padding, 1);

        std::size_t new_size = (bits_used + 7) / 8;
        obs.truncate(new_size);
        if (new_size > obs.size()) {
            println(
                "sloth: compressed file will be larger than original "
                "hahaha");
        }
    }
}

void Serial::Processor::decode(std::string encoded_pathname,
                               std::string pathname) {
    IByteStream ibs(encoded_pathname);
    const uint8_t *ibs_map = ibs.map();

    std::size_t original_size = 0;
    std::memcpy(&original_size, ibs_map, 8);

    uint8_t padding = 0;
    std::memcpy(&padding, ibs_map + 8, 1);

    std::vector<uint8_t> code_sizes;
    std::memcpy(code_sizes.data(), ibs_map + 9, 256);

    std::map<uint16_t, uint8_t> code_map;
    {
        BitVector codes[256];
        {
            std::vector<Symbol> symbols;
            for (std::size_t i = 0; i < 256; ++i) {
                symbols.emplace_back(i, code_sizes[i]);
            }
            symbols.erase(
                std::remove_if(symbols.begin(), symbols.end(),
                               [](const Symbol &s) { return s.length_ == 0; }),
                symbols.end());
            std::sort(symbols.begin(), symbols.end(),
                      [](const Symbol &lhs, const Symbol &rhs) {
                          if (lhs.length_ == rhs.length_) {
                              return lhs.value_ < rhs.value_;
                          }
                          return lhs.length_ < rhs.length_;
                      });
            BitVector code;
            code.zero();
            for (std::size_t i = 0; i < symbols.size() - 1; ++i) {
                codes[symbols[i].value_] = code;
                code.next(symbols[i + 1].length_);
            }
            codes[symbols[symbols.size() - 1].value_] = code;
        }

        for (std::size_t i = 0; i < 256; ++i) {
            BitVector &code = codes[i];
            code_map[code.bits() << (16 - code.size())] = i;
        }
    }
}

void Parallel::Processor::encode(std::string pathname,
                                 std::string encoded_pathname) {}

}  // namespace HuffmanCoding
