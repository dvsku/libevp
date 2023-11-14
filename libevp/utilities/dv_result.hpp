#pragma once

#include <string>

namespace libdvsku {
    enum class dv_status : unsigned char {
        ok            = 0x0,
        error        = 0x1,
        cancelled    = 0x2
    };

    struct dv_result {
        dv_status status    = dv_status::ok;
        std::string msg        = "";

        dv_result() {};
        dv_result(dv_status _status) : status(_status) {};
        dv_result(dv_status _status, const std::string& _msg) : status(_status), msg(_msg) {};

        bool operator!() const {
            return status != dv_status::ok;
        }

        explicit operator bool() const {
            return status == dv_status::ok;
        }
    };
}