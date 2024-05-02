#ifndef BYTE_STREAM_HPP
#define BYTE_STREAM_HPP

#include <cstddef>
#include <cstdint>
#include <string>

class IByteStream {
    uint8_t *bs_;
    std::size_t size_;

   public:
    IByteStream(std::string pathname);
    ~IByteStream();
    std::size_t size() const;
    const uint8_t &operator[](std::size_t index) const;
    const uint8_t *map() const;
};

class OByteStream {
    uint8_t *bs_;
    std::size_t size_;
    int fd_;

   public:
    OByteStream(std::string pathname, std::size_t size);
    ~OByteStream();
    std::size_t size() const;
    uint8_t *truncate(std::size_t size);
    uint8_t *map();
};

#endif
