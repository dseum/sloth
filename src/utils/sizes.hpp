#ifndef SIZES_HPP
#define SIZES_HPP

#include <unistd.h>

namespace Sizes {
static std::size_t page = sysconf(_SC_PAGESIZE);
}  // namespace Sizes

#endif
