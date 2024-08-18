#include "libevp/format/format_v1.hpp"

#include <array>

void libevp::format::v1::format::read_format_desc(libevp::stream_read& stream) {
    std::array<uint8_t, sizeof(HEADER)> header = {};

    stream.seek(0, std::ios::beg);
    stream.read(header.data(), header.size());

    for (size_t i = 0; i < sizeof(HEADER); i++) {
        if (header[i] != HEADER[i])
            return;
    }

    format_type            = static_cast<format::type>(stream.read<uint32_t>());
    file_desc_block_offset = stream.read<uint32_t>();
    file_desc_block_size   = stream.read<uint32_t>();
    file_count             = stream.read<uint64_t>();

    switch (format_type) {
        case format::type::v207: break;
        default: return;
    }

    is_valid = true;
}

void libevp::format::v1::format::read_file_desc_block(libevp::stream_read& stream) {
    auto block = std::make_shared<libevp::format::v1::file_desc_block>();
    desc_block = static_pointer_cast<libevp::format::file_desc_block>(block);

    stream.seek(file_desc_block_offset, std::ios::beg);

    block->region_name_size = stream.read<uint32_t>();
    block->region_name      = stream.read(block->region_name_size);
    block->_unk_1           = stream.read<uint32_t>();
    block->_unk_2           = stream.read<uint32_t>();
    block->_unk_3           = stream.read<uint32_t>();

    for (uint64_t i = 0; i < file_count; i++) {
        evp_fd fd;

        // name size
        uint32_t name_size = stream.read<uint32_t>();

        // name
        fd.file = stream.read(name_size);
        std::replace(fd.file.begin(), fd.file.end(), '\\', '/');

        // data offset
        fd.data_offset = stream.read<uint32_t>();

        // data size
        fd.data_size = stream.read<uint32_t>();

        // unidentified
        stream.seek(0x20);

        block->files.push_back(fd);
    }
}