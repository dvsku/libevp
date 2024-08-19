#include "libevp/evp.hpp"
#include "libevp/format/format.hpp"
#include "libevp/format/supported_formats.hpp"
#include "libevp/stream/stream_read.hpp"
#include "libevp/stream/stream_write.hpp"
#include "libevp/misc/evp_context_internal.hpp"
#include "libevp/utilities/filtering.hpp"
#include "libevp/defs.hpp"
#include "md5/md5.hpp"

#include <thread>
#include <vector>

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
static evp_result validate_evp_archive(const std::filesystem::path& input, bool existing);

/*
    Validate that input is a directory.
*/
static evp_result validate_directory(const std::filesystem::path& input);

/*
    Convert MD5 string to bytes
*/
static void MD5_hex_string_to_bytes(MD5& md5, uint8_t* bytes);

///////////////////////////////////////////////////////////////////////////////
// EVP IMPL

namespace libevp {
    class evp_impl {
    public:
        static evp_result pack_impl(const std::filesystem::path& input_dir, const std::filesystem::path& evp,
            evp_context_internal& context, evp_filter filter = evp_filter::none);

        static evp_result unpack_impl(const std::filesystem::path& evp, const std::filesystem::path& output_dir,
            evp_context_internal& context);
    };
}

///////////////////////////////////////////////////////////////////////////////
// PUBLIC

evp_result evp::pack(const dir_path_t& input_dir, const file_path_t& evp, evp_filter filter) {
    try {
        evp_context_internal context_internal(nullptr);
        return evp_impl::pack_impl(input_dir, evp, context_internal, filter);
    }
    catch (const std::exception& e) {
        evp_result result;
        result.status  = evp_result::status::failure;
        result.message = e.what();

        return result;
    }
}

evp_result evp::unpack(const file_path_t& evp, const dir_path_t& output_dir) {
    try {
        evp_context_internal context_internal(nullptr);
        return evp_impl::unpack_impl(evp, output_dir, context_internal);
    }
    catch (const std::exception& e) {
        evp_result result;
        result.status  = evp_result::status::failure;
        result.message = e.what();

        return result;
    }
}

void evp::pack_async(const dir_path_t& input_dir, const file_path_t& evp, evp_filter filter, evp_context* context) {
    std::thread t([input_dir, evp, filter, context] {
        evp_context_internal context_internal(context);

        try {
            evp_impl::pack_impl(input_dir, evp, context_internal, filter);
        }
        catch (const std::exception& e) {
            evp_result result;
            result.status  = evp_result::status::failure;
            result.message = e.what();

            context_internal.invoke_finish(result);
        }
    });
    t.detach();
}

void evp::unpack_async(const file_path_t& evp, const dir_path_t& output_dir, evp_context* context) {
    std::thread t([evp, output_dir, context] {
        evp_context_internal context_internal(context);

        try {
            evp_impl::unpack_impl(evp, output_dir, context_internal);
        }
        catch (const std::exception& e) {
            evp_result result;
            result.status  = evp_result::status::failure;
            result.message = e.what();

            context_internal.invoke_finish(result);
        }
    });
    t.detach();
}

evp_result evp::validate_files(const file_path_t& evp, std::vector<evp_fd>* failed_fds) {
    evp_result result, res;
    result.status = evp_result::status::failure;

    res = validate_evp_archive(evp, true);
    if (!res) {
        result.message = res.message;
        return result;
    }

    fstream_read stream(evp);
    if (!stream.is_valid()) {
        result.message = "Failed to open file.";
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

            if (failed_fds)
                failed_fds->push_back(file);
        }
    }

    result.status = failed_count == 0 ? evp_result::status::ok : evp_result::status::failure;
    return result;
}

evp_result evp::get_files(const file_path_t& evp, std::vector<evp_fd>& files) {
    evp_result result, res;
    result.status = evp_result::status::failure;

    res = validate_evp_archive(evp, true);
    if (!res) {
        result.message = res.message;
        return result;
    }

    fstream_read stream(evp);
    if (!stream.is_valid()) {
        result.message = "Failed to open file.";
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

evp_result evp::get_file(const file_path_t& evp, const evp_fd& fd, std::vector<uint8_t>& buffer) {
    evp_result result, res;
    result.status = evp_result::status::failure;

    res = validate_evp_archive(evp, true);
    if (!res) {
        result.message = res.message;
        return result;
    }

    fstream_read stream(evp.string());
    if (!stream.is_valid()) {
        result.message = "Failed to open file.";
        return result;
    }

    try {
        buffer.resize(fd.data_size);
        
        stream.seek(fd.data_offset, std::ios::beg);
        stream.read(buffer.data(), fd.data_size);
    }
    catch (const std::exception& e) {
        result.message = e.what();
        return result;
    }

    result.status = evp_result::status::ok;
    return result;
}

evp_result evp::get_file(const file_path_t& evp, const evp_fd& fd, std::stringstream& stream) {
    std::vector<uint8_t> buffer;

    auto result = get_file(evp, fd, buffer);
    if (!result)
        return result;

    std::move(buffer.begin(), buffer.end(), std::ostream_iterator<unsigned char>(stream));

    return result;
}

evp_result evp::get_file(const file_path_t& evp, const file_path_t& file, std::vector<uint8_t>& buffer) {
    evp_result result, res;
    result.status = evp_result::status::failure;
    
    res = validate_evp_archive(evp, true);
    if (!res) {
        result.message = res.message;
        return result;
    }

    fstream_read stream(evp.string());
    if (!stream.is_valid()) {
        result.message = "Failed to open file.";
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
            result.message = "File not found.";
        }
    }
    catch (const std::exception& e) {
        result.message = e.what();
        return result;
    }

    return result;
}

evp_result evp::get_file(const file_path_t& evp, const file_path_t& file, std::stringstream& stream) {
    std::vector<uint8_t> buffer;

    auto result = get_file(evp, file, buffer);
    if (!result)
        return result;

    std::move(buffer.begin(), buffer.end(), std::ostream_iterator<unsigned char>(stream));

    return result;
}

std::vector<evp::file_path_t> evp::get_filtered_files(const dir_path_t& input, evp_filter filter) {
    return filtering::get_filtered_paths(input, filter);
}

///////////////////////////////////////////////////////////////////////////////
// INTERNAL

evp_result read_structure(fstream_read& stream, libevp::format::format::ptr_t& format) {
    evp_result result;
    result.status = evp_result::status::failure;

    try {
        auto res = determine_format(stream, format);
        if (!res) {
            result.message = res.message;
            return result;
        }

        if (!format) {
            result.message = "`format` is null.";
            return result;
        }

        format->read_file_desc_block(stream);
    }
    catch (const std::exception& e) {
        result.message = e.what();
        return result;
    }

    result.status = evp_result::status::ok;
    return result;
}

evp_result determine_format(fstream_read& stream, std::shared_ptr<libevp::format::format>& format) {
    evp_result result;
    result.status = evp_result::status::failure;

    std::shared_ptr<libevp::format::format> supported_formats[] = {
        static_pointer_cast<libevp::format::format>(std::make_shared<libevp::format::v1::format>())
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

evp_result validate_evp_archive(const std::filesystem::path& input, bool existing) {
    evp_result result;
    result.status = evp_result::status::failure;

    try {
        if (existing) {
            if (!std::filesystem::exists(input)) {
                result.message = ".evp file not found.";
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

evp_result validate_directory(const std::filesystem::path& input) {
    evp_result result;
    result.status = evp_result::status::failure;

    try {
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

evp_result evp_impl::pack_impl(const std::filesystem::path& input_dir, const std::filesystem::path& evp,
    evp_context_internal& context, evp_filter filter)
{
    evp_result result, res;
    result.status = evp_result::status::failure;

    ///////////////////////////////////////////////////////////////////////////
    // VERIFY

    std::filesystem::path input  = input_dir;
    std::filesystem::path output = evp;

    if (!input.is_absolute())
        input = std::filesystem::absolute(input);

    if (!output.is_absolute())
        output = std::filesystem::absolute(output);

    res = validate_directory(input);
    if (!res) {
        result.message = res.message;

        context.invoke_finish(result);
        return result;
    }

    res = validate_evp_archive(output, false);
    if (!res) {
        result.message = res.message;

        context.invoke_finish(result);
        return result;
    }

    ///////////////////////////////////////////////////////////////////////////
    // PACK

    format::v1::format format;

    fstream_write stream(output);
    if (!stream.is_valid()) {
        result.message = "Failed to open .evp file.";

        context.invoke_finish(result);
        return result;
    }

    auto  files       = filtering::get_filtered_paths(input, filter);
    float prog_change = 100.0f / files.size();

    context.invoke_start();

    std::vector<uint8_t> buffer{};
    buffer.resize(EVP_BUFFER_SIZE);

    format.write_format_desc(stream);

    for (auto& filename : files) {
        if (context.is_cancelled()) {
            context.invoke_cancel();

            result.status = evp_result::status::cancelled;
            return result;
        }

        fstream_read read_stream(filename);
        if (!read_stream.is_valid()) {
            result.message = "Failed to open file.";

            context.invoke_finish(result);
            return result;
        }

        evp_fd fd;
        fd.file        = filename.string();
        fd.data_offset = (uint32_t)stream.pos();
        fd.data_size   = (uint32_t)read_stream.size();

        {
            std::string base = input.string();

            // convert to relative
            size_t start_index = fd.file.find(base);
            fd.file.erase(start_index, base.size());

            // swap slash direction
            std::replace(fd.file.begin(), fd.file.end(), '/', '\\');

            // remove leading slash
            if (fd.file[0] == '\\')
                fd.file.erase(0, 1);
        }

        MD5      md5;
        uint32_t left_to_read = fd.data_size;

        while (left_to_read > 0) {
            uint32_t read_count = (uint32_t)std::min(left_to_read, EVP_BUFFER_SIZE);

            read_stream.read(buffer.data(), read_count);
            auto pos = stream.pos();
            stream.write(buffer.data(), read_count);

            md5.add(buffer.data(), read_count);

            left_to_read -= read_count;
        }

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

evp_result evp_impl::unpack_impl(const std::filesystem::path& evp, const std::filesystem::path& output_dir,
    evp_context_internal& context)
{
    evp_result result, res;
    result.status = evp_result::status::failure;

    ///////////////////////////////////////////////////////////////////////////
    // VERIFY

    std::filesystem::path input  = evp;
    std::filesystem::path output = output_dir;

    if (!input.is_absolute())
        input = std::filesystem::absolute(input);

    if (!output.is_absolute())
        output = std::filesystem::absolute(output);

    res = validate_evp_archive(evp, true);
    if (!res) {
        result.message = res.message;

        context.invoke_finish(result);
        return result;
    }

    res = validate_directory(output_dir);
    if (!res) {
        result.message = res.message;

        context.invoke_finish(result);
        return result;
    }

    ///////////////////////////////////////////////////////////////////////////
    // UNPACK

    fstream_read stream(input.string());
    if (!stream.is_valid()) {
        result.message = "Failed to open .evp file.";

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

    std::vector<uint8_t> buffer{};
    buffer.resize(EVP_BUFFER_SIZE);

    for (evp_fd& fd : format->desc_block->files) {
        if (context.is_cancelled()) {
            context.invoke_cancel();

            result.status = evp_result::status::cancelled;
            return result;
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
            result.message = "Failed to open file.";
            return result;
        }

        stream.seek(fd.data_offset, std::ios::beg);
        
        uint32_t left_to_read = fd.data_size;
        while (left_to_read > 0) {
            uint32_t read_count = (uint32_t)std::min(left_to_read, EVP_BUFFER_SIZE);

            stream.read(buffer.data(), read_count);
            out_stream.write(buffer.data(), read_count);

            left_to_read -= read_count;
        }

        context.invoke_update(prog_change);
    }

    result.status = evp_result::status::ok;

    context.invoke_finish(result);
    return result;
}
