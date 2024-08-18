#pragma once

#include <libevp/evp_result.hpp>

#include <functional>
#include <atomic>

namespace libevp {
    /*
        evp context object

        Contains user bound callbacks and work cancelling.
    */
    struct evp_context {
        using start_callback_t  = std::function<void()>;
        using finish_callback_t = std::function<void(evp_result)>;
        using update_callback_t = std::function<void(float)>;

        /*
            Start callback.

            @param void()
        */
        start_callback_t start_callback = nullptr;

        /*
            Finish callback.

            @param void(evp_result)
        */
        finish_callback_t finish_callback = nullptr;

        /*
            Update callback.

            @param void(float) -> progress change
        */
        update_callback_t update_callback = nullptr;

        /*
            Cancel token.
        */
        std::atomic_bool* cancel = nullptr;
    };
}
