#pragma once

#include <cstdint>
#include <string>
#include <array>

namespace libevp {
    struct evp_fd {
        std::string             file                 = "";
        uint32_t                data_offset          = 0U;
        uint32_t                data_size            = 0U;
        uint32_t                data_compressed_size = 0U;
        uint32_t                flags                = 0U;
        std::array<uint8_t, 16> hash                 = {};
    };
}
