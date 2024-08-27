#pragma once

#include "libevp/model/evp_fd.hpp"
#include "libevp/stream/stream_read.hpp"

#include <vector>
#include <memory>
#include <cstdint>
#include <functional>

namespace libevp::format {
    struct file_desc_block {
        std::vector<evp_fd> files = {};
    };
    
    struct format {
        using ptr_t          = std::shared_ptr<format>;
        using data_read_cb_t = std::function<void(uint8_t*, uint32_t)>;

        uint32_t file_desc_block_offset = 0U;
        uint32_t file_desc_block_size   = 0U;
        uint32_t file_count             = 0U;
        uint32_t _unk_1                 = 0U;

        bool is_valid = false;

        std::shared_ptr<file_desc_block> desc_block;

        virtual void read_format_desc(libevp::fstream_read& stream)                                        = 0;
        virtual void read_file_desc_block(libevp::fstream_read& stream)                                    = 0;
        virtual void read_file_data(libevp::fstream_read& stream, evp_fd& fd, data_read_cb_t cb = nullptr) = 0;
    };
}
