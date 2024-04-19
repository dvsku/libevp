#pragma once

#include <string>
#include <cstdint>

namespace libevp {
    enum class evp_result_status : uint8_t {
        undefined = 0x0,
        cancelled = 0x1,
        error     = 0x2,
        ok        = 0x3
    };

    struct evp_result {
        evp_result_status status  = evp_result_status::undefined;
        std::string       message = "";

        evp_result() {};

        evp_result(evp_result_status _status)
            : status(_status) {};

        evp_result(evp_result_status _status, const std::string& _message)
            : status(_status), message(_message) {};

        explicit operator bool() const {
            return status == evp_result_status::ok;
        }
    };
}