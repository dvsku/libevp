#pragma once

#include <libevp/evp_result.hpp>

#include <functional>
#include <filesystem>
#include <vector>
#include <cstdint>

namespace libevp {
    class evp;
    class evp_impl;

    class evp_context {
    public:
        using start_callback_t       = std::function<void()>;
        using finish_callback_t      = std::function<void(evp_result)>;
        using update_callback_t      = std::function<void(float)>;
        using buffer_processing_fn_t = std::function<void(const std::filesystem::path& file, std::vector<uint8_t>& buffer)>;

    public:
        start_callback_t       start_callback       = nullptr;
        finish_callback_t      finish_callback      = nullptr;
        update_callback_t      update_callback      = nullptr;
        buffer_processing_fn_t buffer_processing_fn = nullptr;
        bool*                  cancel               = nullptr;

    private:
        friend evp;
        friend evp_impl;

    private:
        void invoke_start();
        void invoke_finish(const evp_result& result);
        void invoke_update(float progress);
        void invoke_buffer_processing(const std::filesystem::path& file, std::vector<uint8_t>& buffer);
        bool invoke_cancel();
    };
}