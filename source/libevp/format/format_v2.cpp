#include "libevp/format/format_v2.hpp"
#include "libevp/stream/stream_write.hpp"
#include "libevp/misc/evp_exception.hpp"
#include "libevp/defs.hpp"

#include <miniz/miniz.h>
#include <array>

////////////////////////////////////////////////////////////////////////////////
// INTERNAL

// TEA encoded size
constexpr uint32_t TEA_CHUNK_SIZE = 64;

// zlib input buffer size (same as non-obfuscated input size)
constexpr uint32_t ZLIB_IN_CHUNK_SIZE = libevp::EVP_READ_CHUNK_SIZE;

// zlib decompress buffer size
constexpr uint32_t ZLIB_OUT_CHUNK_SIZE = ZLIB_IN_CHUNK_SIZE * 4;

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

struct obfuscation {
    bool     encoded           = false;
    bool     compressed        = false;
    uint32_t compressed_size   = 0U;
    uint32_t decompressed_size = 0U;
};

/*
    Read possibly obfuscated block.
    
    Encoded blocks have the first 64 bytes encoded.
    Compressed blocks are compressed by one of the possible compressions.
    
    If block is both encoded and compressed, block was first compressed and then encoded.
    If block is not encoded and compressed, return raw data.

    Possible encodings:
      - TEA

    Possible compressions:
      - zlib
*/
static void read_obfuscated_block(libevp::fstream_read& stream, obfuscation& obfuscation,
    libevp::format::format::data_read_cb_t cb);

/*
    Decode 64 bytes of the block.
*/
static void decode_block(uint8_t* block, uint32_t block_size);

/*
    Check for zlib magic.
*/
static bool zlib_check_magic(uint8_t* data, uint32_t size);

/*
    Decompress zlib block.
*/
static int zlib_decompress_block(mz_stream& stream, uint8_t* src, uint32_t src_size,
    uint8_t* dst, uint32_t dst_size, libevp::format::format::data_read_cb_t cb);

/*
    TEA algorithm decode.
*/
static void TEA_decode(uint8_t input[8], uint8_t output[8], uint32_t* key);

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

    buffer_t buffer = {};

    obfuscation obfuscation       = {};
    obfuscation.encoded           = true;
    obfuscation.compressed        = true;
    obfuscation.compressed_size   = block->compressed_size;
    obfuscation.decompressed_size = block->size;

    read_obfuscated_block(stream, obfuscation, [&](uint8_t* data, uint32_t size) {
        buffer.insert(buffer.end(), data, data + size);
    });

    if ((uint32_t)buffer.size() != block->size)
        throw evp_exception("File desc block decompressed size mismatch.");

    stream_read block_stream(buffer);

    block->region_name_size = block_stream.read<uint32_t>();
    block->region_name      = block_stream.read(block->region_name_size);
    block->_unk_1           = block_stream.read<uint32_t>();
    block->_unk_2           = block_stream.read<uint32_t>();
    block->_unk_3           = block_stream.read<uint32_t>();

    for (uint64_t i = 0; i < file_count; i++) {
        evp_fd fd;

        // name size
        uint32_t name_size = block_stream.read<uint32_t>();

        // name
        fd.file = block_stream.read(name_size);
        std::replace(fd.file.begin(), fd.file.end(), '\\', '/');

        // data offset
        fd.data_offset = block_stream.read<uint32_t>();

        // compressed data size
        fd.data_compressed_size = block_stream.read<uint32_t>();

        // data size
        fd.data_size = block_stream.read<uint32_t>();

        // flags
        fd.flags = block_stream.read<uint32_t>();

        // unidentified
        block_stream.seek(0x8);

        // hash
        block_stream.read(fd.hash.data(), (uint32_t)fd.hash.size());

        block->files.push_back(fd);
    }

    return;
}

void libevp::format::v2::format::read_file_data(libevp::fstream_read& stream, evp_fd& fd, data_read_cb_t cb) {
    if (fd.flags & 4 && fd.data_size == fd.data_compressed_size)
        return;
        //throw evp_exception("Not implemented.");
    
}

////////////////////////////////////////////////////////////////////////////////
// INTERNAL

void read_obfuscated_block(libevp::fstream_read& stream, obfuscation& obfuscation, libevp::format::format::data_read_cb_t cb) {
    libevp::buffer_t read_buf = {};
    read_buf.resize(ZLIB_IN_CHUNK_SIZE);

    libevp::buffer_t decomp_buf = {};
    decomp_buf.resize(ZLIB_OUT_CHUNK_SIZE);

    uint32_t left_to_read = obfuscation.compressed_size;
    uint32_t read_count   = 0U;

    /*
        Read first chunk
    */

    read_count = (uint32_t)std::min(left_to_read, ZLIB_IN_CHUNK_SIZE);
    stream.read(read_buf.data(), read_count);

    if (obfuscation.encoded) {
        decode_block(read_buf.data(), read_count);
    }

    mz_stream mstream{};

    if (obfuscation.compressed) {
        if (!zlib_check_magic(read_buf.data(), read_count))
            throw libevp::evp_exception("Unsupported decompression.");

        mstream.zalloc   = Z_NULL;
        mstream.zfree    = Z_NULL;
        mstream.opaque   = Z_NULL;
        mstream.avail_in = 0;
        mstream.next_in  = Z_NULL;

        if (mz_inflateInit(&mstream) != Z_OK)
            throw libevp::evp_exception("Failed to init inflate stream.");
    }

    do {
        left_to_read -= read_count;

        if (obfuscation.compressed) {
            int res = zlib_decompress_block(mstream, read_buf.data(),
                read_count, decomp_buf.data(), ZLIB_OUT_CHUNK_SIZE, cb);

            if (res == Z_STREAM_END)
                break;

            if (res != 0) {
                mz_inflateEnd(&mstream);
                throw libevp::evp_exception("Failed during decompress.");
            }
        }
        else {
            cb(read_buf.data(), read_count);
        }

        read_count = (uint32_t)std::min(left_to_read, ZLIB_IN_CHUNK_SIZE);
        stream.read(read_buf.data(), read_count);

    } while (left_to_read > 0);

    if (obfuscation.compressed) {
        if (mstream.total_in != obfuscation.compressed_size) {
            mz_inflateEnd(&mstream);
            throw libevp::evp_exception("Failed to decompress. Input not fully read.");
        }

        if (mstream.total_out != obfuscation.decompressed_size) {
            mz_inflateEnd(&mstream);
            throw libevp::evp_exception("Failed to decompress. Output size wrong.");
        }

        mz_inflateEnd(&mstream);
    }
}

void decode_block(uint8_t* block, uint32_t block_size) {
    if (block_size < TEA_CHUNK_SIZE)
        throw libevp::evp_exception("Decode block too small.");

    uint32_t key[4] = {};
    memcpy(key, &KEY, 16);

    for (int i = 0; i < TEA_CHUNK_SIZE / 8; i++) {
        TEA_decode(block + (i * 8), block + (i * 8), key);
    }
}

bool zlib_check_magic(uint8_t* data, uint32_t size) {
    if (size < 2) return false;

    if (data[0] != 0x78)
        return false;

    if (data[1] != 0x01 && data[1] != 0x5E && data[1] != 0x9C && data[1] != 0xDA)
        return false;

    return true;
}

/*
    Adapted from zlib examples.
    https://github.com/madler/zlib/blob/51b7f2abdade71cd9bb0e7a373ef2610ec6f9daf/examples/zpipe.c#L92
*/
int zlib_decompress_block(mz_stream& stream, uint8_t* src, uint32_t src_size,
    uint8_t* dst, uint32_t dst_size, libevp::format::format::data_read_cb_t cb)
{
    if (src_size == 0)
        return Z_DATA_ERROR;

    int      retval            = 0;
    uint32_t decompressed_size = 0U;

    stream.avail_in = src_size;
    stream.next_in  = src;

    do {
        stream.avail_out = dst_size;
        stream.next_out  = dst;

        retval = inflate(&stream, Z_NO_FLUSH);
        if (retval == Z_STREAM_ERROR)
            return retval;

        switch (retval) {
            case Z_NEED_DICT:
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                return retval;

            default: break;
        }

        decompressed_size = dst_size - stream.avail_out;
        cb(dst, decompressed_size);
    } while (stream.avail_out == 0 || stream.avail_in > 0);

    return retval;
}

void TEA_decode(uint8_t input[8], uint8_t output[8], uint32_t* key) {
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
