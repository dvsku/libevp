#include "libevp/evp.hpp"
#include "libevp/format/format.hpp"
#include "libevp/format/supported_formats.hpp"
#include "libevp/stream/stream_read.hpp"
#include "libevp/stream/stream_write.hpp"
#include "libevp/misc/evp_context_internal.hpp"
#include "libevp/utilities/string.hpp"
#include "libevp/defs.hpp"

#include <md5/md5.hpp>
#include <thread>
#include <vector>
#include <unordered_set>

using namespace libevp;

///////////////////////////////////////////////////////////////////////////////
// INTERNAL

/*
    Determines format and reads file descriptors.
*/
static evp_result read_structure(fstream_read& stream, libevp::format::format::ptr_t& format);

/*
    Determine archive format.
*/
static evp_result determine_format(fstream_read& stream, std::shared_ptr<libevp::format::format>& format);

/*
    Validate that input is an EVP archive.
*/
static evp_result validate_evp_archive(const FILE_PATH& input, bool existing);

/*
    Validate that input is a directory.
*/
static evp_result validate_directory(const DIR_PATH& input);

/*
    Convert MD5 string to bytes
*/
static void MD5_hex_string_to_bytes(MD5& md5, uint8_t* bytes);

///////////////////////////////////////////////////////////////////////////////
// EVP IMPL

namespace libevp {
    class evp_impl {
    public:
        static evp_result pack_impl(const evp::pack_input input, FILE_PATH output,
            evp_context_internal& context);

        static evp_result unpack_impl(evp::unpack_input input, DIR_PATH output,
            evp_context_internal& context);
    };
}

///////////////////////////////////////////////////////////////////////////////
// PUBLIC

evp_result evp::pack(const pack_input& input, const FILE_PATH& output) {
    try {
        evp_context_internal context_internal(nullptr);
        return evp_impl::pack_impl(input, output, context_internal);
    }
    catch (const std::exception& e) {
        evp_result result;
        result.status  = evp_result::status::failure;
        result.message = EVP_STR_FORMAT("pack() ex | {}", e.what());

        return result;
    }
}

evp_result evp::unpack(const unpack_input& input, const DIR_PATH& output) {
    try {
        evp_context_internal context_internal(nullptr);
        return evp_impl::unpack_impl(input, output, context_internal);
    }
    catch (const std::exception& e) {
        evp_result result;
        result.status  = evp_result::status::failure;
        result.message = EVP_STR_FORMAT("unpack() ex | {}", e.what());

        return result;
    }
}

void evp::pack_async(const pack_input& input, const FILE_PATH& output, evp_context* context) {
    std::thread t([input, output, context] {
        evp_context_internal context_internal(context);

        try {
            evp_impl::pack_impl(input, output, context_internal);
        }
        catch (const std::exception& e) {
            evp_result result;
            result.status  = evp_result::status::failure;
            result.message = EVP_STR_FORMAT("pack_async() ex | {}", e.what());

            context_internal.invoke_finish(result);
        }
    });
    t.detach();
}

void evp::unpack_async(const unpack_input& input, const DIR_PATH& output, evp_context* context) {
    std::thread t([input, output, context] {
        evp_context_internal context_internal(context);

        try {
            evp_impl::unpack_impl(input, output, context_internal);
        }
        catch (const std::exception& e) {
            evp_result result;
            result.status  = evp_result::status::failure;
            result.message = EVP_STR_FORMAT("unpack_async() ex | {}", e.what());

            context_internal.invoke_finish(result);
        }
    });
    t.detach();
}

evp_result evp::validate_files(const FILE_PATH& input, std::vector<evp_fd>* failed_files) {
    evp_result result, res;
    result.status = evp_result::status::failure;

    res = validate_evp_archive(input, true);
    if (!res) {
        result.message = res.message;
        return result;
    }

    fstream_read stream(input);
    if (!stream.is_valid()) {
        result.message = EVP_STR_FORMAT("Failed to open input archive for reading.");
        return result;
    }

    format::format::ptr_t format;
    res = read_structure(stream, format);
    if (!res) {
        result.message = res.message;
        return result;
    }

    uint32_t failed_count = 0U;
    for (auto& file : format->desc_block->files) {
        MD5                     md5;
        std::array<uint8_t, 16> hash      = {};
        uint32_t                read_size = 0U;

        format->read_file_data(stream, file, [&](uint8_t* data, uint32_t size) {
            md5.add(data, size);
            read_size += size;
        });

        if (read_size)
            MD5_hex_string_to_bytes(md5, hash.data());
        
        if (memcmp(hash.data(), file.hash.data(), 16) != 0) {
            failed_count++;

            if (failed_files)
                failed_files->push_back(file);
        }
    }

    result.status = failed_count == 0 ? evp_result::status::ok : evp_result::status::failure;
    return result;
}

evp_result evp::get_archive_fds(const FILE_PATH& input, std::vector<evp_fd>& files) {
    evp_result result, res;
    result.status = evp_result::status::failure;

    res = validate_evp_archive(input, true);
    if (!res) {
        result.message = res.message;
        return result;
    }

    fstream_read stream(input);
    if (!stream.is_valid()) {
        result.message = EVP_STR_FORMAT("Failed to open input archive for reading.");
        return result;
    }

    format::format::ptr_t format;
    res = read_structure(stream, format);
    if (!res) {
        result.message = res.message;
        return result;
    }

    for (auto& file : format->desc_block->files) {
        files.push_back(file);
    }

    result.status = evp_result::status::ok;
    return result;
}

evp_result evp::get_file(const FILE_PATH& input, const evp_fd& file, std::vector<uint8_t>& buffer) {
    evp_result result, res;
    result.status = evp_result::status::failure;

    res = validate_evp_archive(input, true);
    if (!res) {
        result.message = res.message;
        return result;
    }

    fstream_read stream(input);
    if (!stream.is_valid()) {
        result.message = EVP_STR_FORMAT("Failed to open input archive for reading.");
        return result;
    }

    try {
        buffer.resize(file.data_size);
        
        stream.seek(file.data_offset, std::ios::beg);
        stream.read(buffer.data(), file.data_size);
    }
    catch (const std::exception& e) {
        result.message = e.what();
        return result;
    }

    result.status = evp_result::status::ok;
    return result;
}

evp_result evp::get_file(const FILE_PATH& input, const evp_fd& file, std::stringstream& stream) {
    buffer_t buffer;

    auto result = get_file(input, file, buffer);
    if (!result)
        return result;

    std::move(buffer.begin(), buffer.end(), std::ostream_iterator<unsigned char>(stream));

    return result;
}

evp_result evp::get_file(const FILE_PATH& input, const FILE_PATH& file, std::vector<uint8_t>& buffer) {
    evp_result result, res;
    result.status = evp_result::status::failure;
    
    res = validate_evp_archive(input, true);
    if (!res) {
        result.message = res.message;
        return result;
    }

    fstream_read stream(input);
    if (!stream.is_valid()) {
        result.message = EVP_STR_FORMAT("Failed to open input archive for reading.");
        return result;
    }

    format::format::ptr_t format;
    res = read_structure(stream, format);
    if (!res) {
        result.message = res.message;
        return result;
    }

    try {
        bool found = false;

        for (evp_fd& fd : format->desc_block->files) {
            if (fd.file != file) continue;

            buffer.resize(fd.data_size);

            stream.seek(fd.data_offset, std::ios::beg);
            stream.read(buffer.data(), fd.data_size);

            found = true;
        }

        if (found) {
            result.status = evp_result::status::ok;
        }
        else {
            result.message = EVP_STR_FORMAT("File not found.");
        }
    }
    catch (const std::exception& e) {
        result.message = e.what();
        return result;
    }

    return result;
}

evp_result evp::get_file(const FILE_PATH& input, const FILE_PATH& file, std::stringstream& stream) {
    buffer_t buffer;

    auto result = get_file(input, file, buffer);
    if (!result)
        return result;

    std::move(buffer.begin(), buffer.end(), std::ostream_iterator<unsigned char>(stream));

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// INTERNAL

evp_result read_structure(fstream_read& stream, libevp::format::format::ptr_t& format) {
    evp_result result;
    result.status = evp_result::status::failure;

    try {
        auto res = determine_format(stream, format);
        if (!res) {
            result.message = EVP_STR_FORMAT("Archive format not supported.");
            return result;
        }

        if (!format) {
            result.message = EVP_STR_FORMAT("Archive format not supported.");
            return result;
        }

        format->read_file_desc_block(stream);
    }
    catch (const std::exception& e) {
        result.message = EVP_STR_FORMAT("read_structure() ex | {}", e.what());
        return result;
    }

    result.status = evp_result::status::ok;
    return result;
}

evp_result determine_format(fstream_read& stream, std::shared_ptr<libevp::format::format>& format) {
    evp_result result;
    result.status = evp_result::status::failure;

    std::shared_ptr<libevp::format::format> supported_formats[] = {
        static_pointer_cast<libevp::format::format>(std::make_shared<libevp::format::v1::format>()),
        static_pointer_cast<libevp::format::format>(std::make_shared<libevp::format::v2::format>()),
    };

    for (std::shared_ptr<libevp::format::format> supported_format : supported_formats) {
        if (!supported_format) continue;
        
        supported_format->read_format_desc(stream);
        
        if (supported_format->is_valid) {
            format = supported_format;

            result.status = evp_result::status::ok;
            return result;
        }
    }

    return result;
}

evp_result validate_evp_archive(const FILE_PATH& input, bool existing) {
    evp_result result;
    result.status = evp_result::status::failure;

    try {
        if (existing) {
            if (!std::filesystem::exists(input)) {
                result.message = "File not found.";
                return result;
            }

            if (!std::filesystem::is_regular_file(input)) {
                result.message = "Not a file.";
                return result;
            }
        }

        if (!input.has_filename()) {
            result.message = "Not a file with .evp extension.";
            return result;
        }

        if (!input.has_extension() || input.extension() != ".evp") {
            result.message = "Not a file with .evp extension.";
            return result;
        }
    }
    catch (const std::exception& e) {
        result.message = e.what();
        return result;
    }

    result.status = evp_result::status::ok;
    return result;
}

evp_result validate_directory(const DIR_PATH& input) {
    evp_result result;
    result.status = evp_result::status::failure;

    try {
        if (input == "") {
            result.message = "Cannot be empty.";
            return result;
        }

        if (!std::filesystem::exists(input)) {
            result.message = "Directory not found.";
            return result;
        }

        if (!std::filesystem::is_directory(input)) {
            result.message = "Not a directory.";
            return result;
        }
    }
    catch (const std::exception& e) {
        result.message = e.what();
        return result;
    }

    result.status = evp_result::status::ok;
    return result;
}

void MD5_hex_string_to_bytes(MD5& md5, uint8_t* bytes) {
    std::string hex_bytes = md5.getHash();

    for (size_t i = 0; i < hex_bytes.size(); i += 2) {
        uint8_t high = std::isdigit(hex_bytes[i])     ? (hex_bytes[i]     - '0') : (std::toupper(hex_bytes[i])     - 'A' + 10);
        uint8_t low  = std::isdigit(hex_bytes[i + 1]) ? (hex_bytes[i + 1] - '0') : (std::toupper(hex_bytes[i + 1]) - 'A' + 10);

        bytes[i / 2] = (high << 4) | low;
    }
}

///////////////////////////////////////////////////////////////////////////////
// EVP IMPL

evp_result evp_impl::pack_impl(const evp::pack_input input, FILE_PATH output,
    evp_context_internal& context)
{
    evp_result result, res;
    result.status = evp_result::status::failure;

    ///////////////////////////////////////////////////////////////////////////
    // VERIFY

    if (!output.is_absolute())
        output = std::filesystem::absolute(output);

    res = validate_directory(input.base);
    if (!res) {
        result.message = EVP_STR_FORMAT("Failed to validate input base path. | {}", res.message);

        context.invoke_finish(result);
        return result;
    }

    res = validate_evp_archive(output, false);
    if (!res) {
        result.message = EVP_STR_FORMAT("Failed to validate output archive path. | {}", res.message);

        context.invoke_finish(result);
        return result;
    }

    ///////////////////////////////////////////////////////////////////////////
    // PACK

    format::v1::format format;

    fstream_write stream(output);
    if (!stream.is_valid()) {
        result.message = EVP_STR_FORMAT("Failed to open output archive file for writing.");

        context.invoke_finish(result);
        return result;
    }

    float prog_change = 100.0f / input.files.size();

    context.invoke_start();

    buffer_t buffer{};
    buffer.resize(EVP_READ_CHUNK_SIZE);

    format.write_format_desc(stream);

    for (const auto& relative_file : input.files) {
        if (context.is_cancelled()) {
            context.invoke_cancel();

            result.status = evp_result::status::cancelled;
            return result;
        }

        std::filesystem::path file = input.base;
        file /= relative_file;

        if (!std::filesystem::exists(file)) {
            result.message = EVP_STR_FORMAT("`{}` | File not found.", file.string().c_str());

            context.invoke_finish(result);
            return result;
        }

        fstream_read read_stream(file);
        if (!read_stream.is_valid()) {
            result.message = EVP_STR_FORMAT("`{}` | Failed to open file for reading.", file.string().c_str());

            context.invoke_finish(result);
            return result;
        }

        evp_fd fd;
        fd.file        = relative_file.string();
        fd.data_offset = (uint32_t)stream.pos();
        fd.data_size   = (uint32_t)read_stream.size();

        // Swap slash direction
        std::replace(fd.file.begin(), fd.file.end(), '/', '\\');

        // Remove leading slash
        if (fd.file[0] == '\\')
            fd.file.erase(0, 1);

        MD5      md5;
        uint32_t left_to_read = fd.data_size;

        while (left_to_read > 0) {
            // read file chunk
            uint32_t read_count = (uint32_t)std::min(left_to_read, EVP_READ_CHUNK_SIZE);
            read_stream.read(buffer.data(), read_count);
            
            // write file chunk to archive
            stream.write(buffer.data(), read_count);

            // compute chunk MD5
            md5.add(buffer.data(), read_count);

            left_to_read -= read_count;
        }

        // compute file MD5
        MD5_hex_string_to_bytes(md5, fd.hash.data());

        format.desc_block->files.push_back(fd);
        context.invoke_update(prog_change);
    }

    format.file_desc_block_offset = (uint32_t)stream.pos();
    format.file_count             = (uint64_t)format.desc_block->files.size();
    
    format.write_file_desc_block(stream);
    format.write_format_desc(stream);

    result.status = evp_result::status::ok;

    context.invoke_finish(result);
    return result;
}

evp_result evp_impl::unpack_impl(evp::unpack_input input, DIR_PATH output,
    evp_context_internal& context)
{
    evp_result result, res;
    result.status = evp_result::status::failure;

    ///////////////////////////////////////////////////////////////////////////
    // VERIFY

    if (!input.archive.is_absolute())
        input.archive = std::filesystem::absolute(input.archive);

    if (!output.is_absolute())
        output = std::filesystem::absolute(output);

    res = validate_evp_archive(input.archive, true);
    if (!res) {
        result.message = EVP_STR_FORMAT("Failed to validate input archive path. | {}", res.message);

        context.invoke_finish(result);
        return result;
    }

    res = validate_directory(output);
    if (!res) {
        result.message = EVP_STR_FORMAT("Failed to validate output path. | {}", res.message);

        context.invoke_finish(result);
        return result;
    }

    std::unordered_set<uint32_t> requested_fds = {};
    for (evp_fd& fd : input.files) {
        requested_fds.insert(fd.data_offset);
    }

    ///////////////////////////////////////////////////////////////////////////
    // UNPACK

    fstream_read stream(input.archive);
    if (!stream.is_valid()) {
        result.message = EVP_STR_FORMAT("Failed to open input archive for reading.");

        context.invoke_finish(result);
        return result;
    }

    format::format::ptr_t format;
    res = read_structure(stream, format);
    if (!res) {
        result.message = res.message;

        context.invoke_finish(result);
        return result;
    }

    float prog_change = 100.0f / format->file_count;

    context.invoke_start();

    for (evp_fd& fd : format->desc_block->files) {
        if (context.is_cancelled()) {
            context.invoke_cancel();

            result.status = evp_result::status::cancelled;
            return result;
        }
        
        if (requested_fds.size() != 0 && !requested_fds.contains(fd.data_offset)) {
            context.invoke_update(prog_change);
            continue;
        }

        std::filesystem::path dir_path(output);
        dir_path /= fd.file;
        dir_path.remove_filename();

        std::filesystem::path file_path(output);
        file_path /= fd.file;

        if (!std::filesystem::is_directory(dir_path)) {
            std::filesystem::create_directories(dir_path);
            std::filesystem::permissions(dir_path, std::filesystem::perms::all);
        }

        fstream_write out_stream(file_path);
        if (!out_stream.is_valid()) {
            result.message = EVP_STR_FORMAT("`{}` | Failed to open file for writing.", file_path.string().c_str());
            return result;
        }

        format->read_file_data(stream, fd, [&](uint8_t* data, uint32_t size) {
            out_stream.write(data, size);
        });

        context.invoke_update(prog_change);
    }

    result.status = evp_result::status::ok;

    context.invoke_finish(result);
    return result;
}
