#pragma once

#include "libevp/format/format.hpp"

#include <string>

namespace libevp::format::v2 {
    struct file_desc_block : libevp::format::file_desc_block {
        uint32_t    size             = 0U;
        uint32_t    compressed_size  = 0U;

        uint32_t    region_name_size = 0U;
        std::string region_name      = "";
        uint32_t    _unk_1           = 0U;
        uint32_t    _unk_2           = 0U;
        uint32_t    _unk_3           = 0U;
    };

    class format : public libevp::format::format {
    public:
        using file_desc_block_ptr_t = std::shared_ptr<file_desc_block>;

        enum class type : uint32_t {
            undefined = 0x00000000,
            v101      = 0x00000065,
            v102      = 0x00000066
        };

    public:
        format::type format_type = format::type::undefined;

    public:
        format();

    public:
        void read_format_desc(libevp::fstream_read& stream)                                        override final;
        void read_file_desc_block(libevp::fstream_read& stream)                                    override final;
        void read_file_data(libevp::fstream_read& stream, evp_fd& fd, data_read_cb_t cb = nullptr) override final;
    };
}
