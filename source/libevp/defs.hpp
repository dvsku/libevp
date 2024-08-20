#pragma once

#include <vector>
#include <cstdint>

namespace libevp {
    constexpr uint32_t EVP_READ_CHUNK_SIZE = 16 * 1024;

    using buffer_t = std::vector<uint8_t>;
}
