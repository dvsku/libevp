#pragma once

#include <stdexcept>

namespace libevp {
    class evp_exception : std::runtime_error {
    public:
        evp_exception(const std::string& message)
            : std::runtime_error(message) {}
    };
}
