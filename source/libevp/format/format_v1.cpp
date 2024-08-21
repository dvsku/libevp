#include "libevp/format/format_v1.hpp"
#include "libevp/defs.hpp"

#include <array>

////////////////////////////////////////////////////////////////////////////////
// INTERNAL

constexpr uint8_t HEADER[56] = {
    0x35, 0x32, 0x35, 0x63, 0x31, 0x37, 0x61, 0x36, 0x61, 0x37, 0x63, 0x66, 0x62, 0x63,
    0x64, 0x37, 0x35, 0x34, 0x31, 0x32, 0x65, 0x63, 0x64, 0x30, 0x36, 0x39, 0x64, 0x34,
    0x62, 0x37, 0x32, 0x63, 0x33, 0x38, 0x39, 0x00, 0x10, 0x00, 0x00, 0x00, 0x4E, 0x4F,
    0x52, 0x4D, 0x41, 0x4C, 0x5F, 0x50, 0x41, 0x43, 0x4B, 0x5F, 0x54, 0x59, 0x50, 0x45
};

////////////////////////////////////////////////////////////////////////////////
// PUBLIC

libevp::format::v1::format::format() {
    desc_block = std::make_shared<libevp::format::v1::file_desc_block>();
}

void libevp::format::v1::format::read_format_desc(libevp::fstream_read& stream) {
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
    file_count             = stream.read<uint32_t>();
    _unk_1                 = stream.read<uint32_t>();

    switch (format_type) {
        case format::type::v100: break;
        default: return;
    }

    is_valid = true;
}

void libevp::format::v1::format::read_file_desc_block(libevp::fstream_read& stream) {
    if (!desc_block) return;

    file_desc_block_ptr_t block = static_pointer_cast<file_desc_block>(desc_block);

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

        // compressed data size
        fd.data_compressed_size = stream.read<uint32_t>();

        // data size
        fd.data_size = stream.read<uint32_t>();

        // flags
        fd.flags = stream.read<uint32_t>();

        // unidentified
        stream.seek(0x8);

        // hash
        stream.read(fd.hash.data(), (uint32_t)fd.hash.size());

        block->files.push_back(fd);
    }
}

void libevp::format::v1::format::read_file_data(libevp::fstream_read& stream, evp_fd& fd, data_read_cb_t cb) {
    stream.seek(fd.data_offset, std::ios::beg);

    buffer_t buffer = {};
    buffer.resize(EVP_READ_CHUNK_SIZE);

    uint32_t left_to_read = fd.data_size;
    while (left_to_read > 0) {
        uint32_t read_count = (uint32_t)std::min(left_to_read, EVP_READ_CHUNK_SIZE);

        stream.read(buffer.data(), read_count);
        if (cb)
            cb(buffer.data(), read_count);

        left_to_read -= read_count;
    }
}

void libevp::format::v1::format::write_format_desc(libevp::fstream_write& stream) {
    stream.seek(0, std::ios::beg);

    stream.write((uint8_t*)HEADER, (uint32_t)sizeof(HEADER));
    stream.write(format::type::v100);
    stream.write(file_desc_block_offset);
    stream.write(file_desc_block_size);
    stream.write(file_count);
    stream.write(_unk_1);
}

void libevp::format::v1::format::write_file_desc_block(libevp::fstream_write& stream) {
    if (!desc_block) return;

    file_desc_block_ptr_t block = static_pointer_cast<file_desc_block>(desc_block);

    stream.seek(file_desc_block_offset, std::ios::beg);   
    file_desc_block_size = (uint32_t)stream.pos();
    
    stream.write(block->region_name);
    stream.write(block->_unk_1);
    stream.write(block->_unk_2);
    stream.write(block->_unk_3);

    for (size_t i = 0; i < block->files.size(); i++) {
        evp_fd& fd = block->files[i];

        stream.write(fd.file);
        stream.write(fd.data_offset);
        stream.write(fd.data_size);
        stream.write(fd.data_size);
        stream.write((uint32_t)0x00000001);
        stream.write((uint32_t)0x00000000);
        stream.write((uint32_t)0x00000000);
        stream.write(fd.hash.data(), (uint32_t)fd.hash.size());
    }

    file_desc_block_size = (uint32_t)stream.pos() - file_desc_block_size;
}
