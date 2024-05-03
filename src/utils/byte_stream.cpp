#include "byte_stream.hpp"

#include <fcntl.h>
#define _GNU_SOURCE
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

#include "print.hpp"

/**
 * IByteStream
 */

IByteStream::IByteStream(std::string pathname) {
    const char *pathname_c = pathname.c_str();
    int fd;
    if ((fd = open(pathname_c, O_RDONLY)) == -1) {
        println("Error: {}", strerror(errno));
        exit(EXIT_FAILURE);
    }
    struct stat st;
    stat(pathname_c, &st);
    size_ = st.st_size;
    bs_ = static_cast<uint8_t *>(mmap(0, size_, PROT_READ, MAP_PRIVATE, fd, 0));
    if (bs_ == MAP_FAILED) {
        println("Error: {}", strerror(errno));
        exit(EXIT_FAILURE);
    }
    close(fd);
}

IByteStream::~IByteStream() { munmap(bs_, size_); }

std::size_t IByteStream::size() const { return size_; }

const uint8_t &IByteStream::operator[](std::size_t index) const {
    return bs_[index];
}

const uint8_t *IByteStream::map() const { return bs_; }

/**
 * OByteStream
 */

OByteStream::OByteStream(std::string pathname, std::size_t size) : size_(size) {
    const char *pathname_c = pathname.c_str();
    if ((fd_ = open(pathname_c, O_RDWR | O_CREAT | O_TRUNC, 0644)) == -1) {
        println("Error: {}", strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (ftruncate(fd_, size_) == -1) {
        println("Error: {}", strerror(errno));
        exit(EXIT_FAILURE);
    }
    bs_ = static_cast<uint8_t *>(
        mmap(0, size_, PROT_WRITE, MAP_SHARED, fd_,
             0));  // assume that write allows happens on disjoint regions
    if (bs_ == MAP_FAILED) {
        println("Error: {}", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

OByteStream::~OByteStream() {
    munmap(bs_, size_);
    close(fd_);
}

std::size_t OByteStream::size() const { return size_; }

uint8_t *OByteStream::map() { return bs_; }
