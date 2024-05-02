#ifndef HUFFMAN_CODING_HPP
#define HUFFMAN_CODING_HPP

#include <cstddef>
#include <cstdint>
#include <queue>
#include <string>
#include <utility>

#include "utils/bit_vector.hpp"

namespace HuffmanCoding {

struct Symbol {
    uint8_t value_ = 0;
    uint8_t length_ = 0;
};

struct Package {
    std::vector<Package *> within_;
    std::size_t weight_;
    uint8_t value_;
    uint8_t length_ = 0;

    Package();
    Package(uint8_t value, std::size_t weight, Package *self = nullptr);
    bool operator<(const Package &rhs) const;
    void add(Package &rhs);
};

struct Node {
    Node *left_;
    Node *right_;
    uint8_t value_;
    BitVector code_;

    Node();
    Node(uint8_t value, std::size_t weight, Node *left = nullptr,
         Node *right = nullptr);
    bool operator<(const Node &rhs) const;
    bool operator==(const Node &rhs) const;
};
using PackageArray = std::pair<Package *, std::size_t>;
using NodeArray = std::pair<Node *, std::size_t>;

class PriorityQueue {
    struct Comparator {
        bool operator()(const Node *lhs, const Node *rhs) const;
    };

   public:
    using data_type =
        std::priority_queue<Node *, std::vector<Node *>, Comparator>;
    PriorityQueue(NodeArray nodes);
    Node *pop();
    std::pair<Node *, Node *> pop_pair();
    void push(Node *node);
    data_type::size_type size() const;

   private:
    data_type pq_;
};

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
