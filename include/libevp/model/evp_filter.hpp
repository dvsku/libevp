#pragma once

#include <cstdint>

namespace libevp {
    enum class evp_filter : uint32_t {
        none        = 0x00,   // include all files
        client_only = 0x01,   // include only Talisman Online client files
        server_only = 0x02    // include only Talisman Online server files
    };
}
