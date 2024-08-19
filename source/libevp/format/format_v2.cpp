#include "libevp/format/format_v2.hpp"
#include "libevp/stream/stream_write.hpp"
#include "libevp/misc/evp_exception.hpp"

#include <miniz/miniz.h>
#include <array>

////////////////////////////////////////////////////////////////////////////////
// INTERNAL

constexpr uint8_t HEADER[56] = {
    0x35, 0x32, 0x35, 0x63, 0x31, 0x37, 0x61, 0x36, 0x61, 0x37, 0x63, 0x66, 0x62, 0x63,
    0x64, 0x37, 0x35, 0x34, 0x31, 0x32, 0x65, 0x63, 0x64, 0x30, 0x36, 0x39, 0x64, 0x34,
    0x62, 0x37, 0x32, 0x63, 0x33, 0x38, 0x39, 0x00, 0x10, 0x00, 0x00, 0x00, 0x4E, 0x4F,
    0x52, 0x4D, 0x41, 0x4C, 0x5F, 0x50, 0x41, 0x43, 0x4B, 0x5F, 0x54, 0x59, 0x50, 0x45
};

constexpr uint8_t KEY[] = {
    0x41, 0xF5, 0xDF, 0x98, 0xC2, 0x05, 0x48, 0x2B,
    0x9B, 0x97, 0xAF, 0x01, 0xA5, 0x4B, 0x14, 0xD8
};

/*
    TEA algorithm decode
*/
static void internal_decode(uint8_t input[8], uint8_t output[8], uint32_t* key);

////////////////////////////////////////////////////////////////////////////////
// PUBLIC

libevp::format::v2::format::format() {
    desc_block = std::make_shared<libevp::format::v2::file_desc_block>();
}

void libevp::format::v2::format::read_format_desc(libevp::fstream_read& stream) {
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
        case format::type::v207_1: break;
        default: return;
    }

    is_valid = true;
}

void libevp::format::v2::format::read_file_desc_block(libevp::fstream_read& stream) {
    if (!desc_block) return;

    file_desc_block_ptr_t block = static_pointer_cast<file_desc_block>(desc_block);

    stream.seek(file_desc_block_offset, std::ios::beg);

    block->size            = stream.read<uint32_t>();
    block->compressed_size = stream.read<uint32_t>();

    if (block->size == block->compressed_size)
        throw evp_exception("Not implemented: File desc block not compressed.");

    std::vector<uint8_t> decompressed_buffer = {};

    {
        std::vector<uint8_t> compressed_buffer = {};

        decode_file_desc_block(stream, compressed_buffer);
        uint32_t decoded_size = (uint32_t)compressed_buffer.size();

        compressed_buffer.resize(block->compressed_size);
        stream.read(compressed_buffer.data() + decoded_size, block->compressed_size - decoded_size);

        decompressed_buffer.resize(block->size);
        decompress_file_desc_block(compressed_buffer, decompressed_buffer);
    }

    stream_read decompressed_stream(decompressed_buffer);

    block->region_name_size = decompressed_stream.read<uint32_t>();
    block->region_name      = decompressed_stream.read(block->region_name_size);
    block->_unk_1           = decompressed_stream.read<uint32_t>();
    block->_unk_2           = decompressed_stream.read<uint32_t>();
    block->_unk_3           = decompressed_stream.read<uint32_t>();

    for (uint64_t i = 0; i < file_count; i++) {
        evp_fd fd;

        // name size
        uint32_t name_size = decompressed_stream.read<uint32_t>();

        // name
        fd.file = decompressed_stream.read(name_size);
        std::replace(fd.file.begin(), fd.file.end(), '\\', '/');

        // data offset
        fd.data_offset = decompressed_stream.read<uint32_t>();

        // compressed data size
        fd.data_compressed_size = decompressed_stream.read<uint32_t>();

        // data size
        fd.data_size = decompressed_stream.read<uint32_t>();

        // flags
        fd.flags = decompressed_stream.read<uint32_t>();

        // unidentified
        decompressed_stream.seek(0x8);

        // hash
        decompressed_stream.read(fd.hash.data(), (uint32_t)fd.hash.size());

        block->files.push_back(fd);
    }

    return;
}

void libevp::format::v2::format::read_file_data(libevp::fstream_read& stream, evp_fd& fd, data_read_cb_t cb) {
    throw evp_exception("Not implemented.");
}

////////////////////////////////////////////////////////////////////////////////
// PRIVATE

void libevp::format::v2::format::decode_file_desc_block(libevp::fstream_read& stream, std::vector<uint8_t>& buffer) {
    file_desc_block_ptr_t block       = static_pointer_cast<file_desc_block>(desc_block);
    uint32_t              decode_size = 64;

    if (decode_size >= block->compressed_size) {
        decode_size  =  block->compressed_size;
        decode_size -=  1;
        decode_size &= -8;
    }

    uint32_t key[4] = {};
    memcpy(key, &KEY, 16);

    for (int i = 0; decode_size > 0; i++) {
        uint32_t read_count = std::min(decode_size, (uint32_t)8);
        uint8_t  input[8]   = {};
        uint8_t  output[8]  = {};

        stream.read(input, read_count);
        internal_decode(input, output, key);

        /*
            Check for zlib magic.
            Exception thrown if key was changed or compression is not zlib.
        */

        if (i == 0) {
            if (output[0] != 0x78)
                throw evp_exception("Wrong decode key or unsupported compression.");

            if (output[1] != 0x01 && output[1] != 0x5E && output[1] != 0x9C && output[1] != 0xDA)
                throw evp_exception("Wrong decode key or unsupported compression.");
        }

        buffer.insert(buffer.end(), output, output + read_count);
        decode_size -= read_count;
    }
}

void libevp::format::v2::format::decompress_file_desc_block(std::vector<uint8_t>& input, std::vector<uint8_t>& output) {
    file_desc_block_ptr_t block = static_pointer_cast<file_desc_block>(desc_block);

    mz_ulong decompressed_size = static_cast<mz_ulong>(block->size);

    int code = mz_uncompress(output.data(), &decompressed_size, input.data(),
        static_cast<mz_ulong>(block->compressed_size));

    if (code != MZ_OK)
        throw evp_exception("Unsupported compression.");

    if (block->size != decompressed_size)
        throw evp_exception("Unsupported compression.");
}

////////////////////////////////////////////////////////////////////////////////
// INTERNAL

void internal_decode(uint8_t input[8], uint8_t output[8], uint32_t* key) {
    uint32_t delta  = 0x9E3779B9;
    uint32_t sum    = 0xC6EF3720;
    uint32_t cycles = 32;
    
    uint32_t v0 = ((uint32_t)input[0])
                | ((uint32_t)input[1] << 8)
                | ((uint32_t)input[2] << 16)
                | ((uint32_t)input[3] << 24);
    
    uint32_t v1 = ((uint32_t)input[4])
                | ((uint32_t)input[5] << 8)
                | ((uint32_t)input[6] << 16)
                | ((uint32_t)input[7] << 24);

    for (uint32_t i = 0; i < cycles; i++) {   
        v1 -= ((v0 << 4) + key[2]) ^ (v0 + sum) ^ ((v0 >> 5) + key[3]);
        v0 -= ((v1 << 4) + key[0]) ^ (v1 + sum) ^ ((v1 >> 5) + key[1]);

        sum -= delta;
    }

    output[0] = (uint8_t)(v0);
    output[1] = (uint8_t)(v0 >> 8);
    output[2] = (uint8_t)(v0 >> 16);
    output[3] = (uint8_t)(v0 >> 24);
    output[4] = (uint8_t)(v1);
    output[5] = (uint8_t)(v1 >> 8);
    output[6] = (uint8_t)(v1 >> 16);
    output[7] = (uint8_t)(v1 >> 24);
}
