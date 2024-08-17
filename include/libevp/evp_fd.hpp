#pragma once

#include <cstdint>
#include <string>

namespace libevp {
    struct evp_fd {
        std::string file        = "";
        uint32_t    data_offset = 0U;
        uint32_t    data_size   = 0U;
    };
}
