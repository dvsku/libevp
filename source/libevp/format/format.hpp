#pragma once

#include "libevp/evp_fd.hpp"
#include "libevp/stream/stream_read.hpp"

#include <vector>
#include <memory>
#include <cstdint>

namespace libevp::format {
    struct file_desc_block {
        std::vector<evp_fd> files = {};
    };
    
    struct format {
        using ptr_t = std::shared_ptr<format>;

        uint32_t file_desc_block_offset = 0U;
        uint32_t file_desc_block_size   = 0U;
        uint64_t file_count             = 0U;

        bool is_valid = false;

        std::shared_ptr<file_desc_block> desc_block;

        virtual void read_format_desc(libevp::stream_read& stream)     = 0;
        virtual void read_file_desc_block(libevp::stream_read& stream) = 0;
    };
}
