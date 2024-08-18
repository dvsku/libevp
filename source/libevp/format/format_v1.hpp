#pragma once

#include "libevp/format/format.hpp"
#include "libevp/stream/stream_write.hpp"

#include <string>

namespace libevp::v1 {
    class format_desc {
    public:
        static inline char HEADER[60] = {
            53,50,53,99,49,55,97,54,97,55,99,102,98,99,100,55,53,52,49,50,101,99,100,48,54,57,100,52,
            98,55,50,99,51,56,57,0,16,0,0,0,78,79,82,77,65,76,95,80,65,67,75,95,84,89,80,69,100,0,0,0
        };

        static inline char RESERVED[16] = {
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        };
    };

    static_assert(sizeof(format_desc) != 76, "libevp::v1::format_desc wrong size");

    static inline unsigned int HEADER_END_OFFSET        = 0x3C;
    static inline unsigned int DATA_START_OFFSET        = 0x4C;
    static inline unsigned int GAP_BETWEEN_FILE_DESC    = 0x24;
}

namespace libevp::format::v1 {
    struct file_desc_block : libevp::format::file_desc_block {
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
            v207      = 0x00000064
        };

    public:
        format::type format_type = format::type::undefined;

    public:
        format();
        
    public:
        void read_format_desc(libevp::stream_read& stream)     override final;
        void read_file_desc_block(libevp::stream_read& stream) override final;

        void write_format_desc(libevp::stream_write& stream);
        void write_file_desc_block(libevp::stream_write& stream);

    private:
        static inline uint8_t HEADER[56] = {
            0x35, 0x32, 0x35, 0x63, 0x31, 0x37, 0x61, 0x36, 0x61, 0x37, 0x63, 0x66, 0x62, 0x63,
            0x64, 0x37, 0x35, 0x34, 0x31, 0x32, 0x65, 0x63, 0x64, 0x30, 0x36, 0x39, 0x64, 0x34,
            0x62, 0x37, 0x32, 0x63, 0x33, 0x38, 0x39, 0x00, 0x10, 0x00, 0x00, 0x00, 0x4E, 0x4F,
            0x52, 0x4D, 0x41, 0x4C, 0x5F, 0x50, 0x41, 0x43, 0x4B, 0x5F, 0x54, 0x59, 0x50, 0x45
        };
    };
}
