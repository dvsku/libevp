#pragma once

#include <string>
#include <cstdint>

namespace libevp {
    struct evp_result {
        enum class status : uint8_t {
            undefined = 0x0,
            cancelled = 0x1,
            failure   = 0x2,
            ok        = 0x3
        };

        evp_result::status status  = evp_result::status::undefined;
        std::string        message = "";

        explicit operator bool() const {
            return status == evp_result::status::ok;
        }
    };
}
