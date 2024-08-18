#include "libevp/evp.hpp"
#include "libevp/utilities/filtering.hpp"
#include "libevp/format/format.hpp"
#include "libevp/format/supported_formats.hpp"
#include "libevp/stream/stream_read.hpp"
#include "libevp/stream/stream_write.hpp"
#include "md5/md5.hpp"

#include <thread>
#include <vector>
#include <array>
#include <fstream>
#include <iterator>

using namespace libevp;

constexpr uint32_t EVP_BUFFER_SIZE = 1024 * 1024;

///////////////////////////////////////////////////////////////////////////////
// INTERNAL

struct file_desc {
    std::string             path = "";
    std::vector<uint8_t>    data{};
    std::array<uint8_t, 16> data_hash{};
    uint32_t                data_size = 0;
    uint32_t                path_size = 0;
    uint32_t                data_start_offset = 0;
};

/*
    Determines format and reads file descriptors.
*/
static evp_result read_structure(stream_read& stream, libevp::format::format::ptr_t& format);

/*
    Determine archive format.
*/
static evp_result determine_format(stream_read& stream, std::shared_ptr<libevp::format::format>& format);

/*
    Validate that input is an EVP archive.
*/
static evp_result validate_evp_archive(const std::filesystem::path& input, bool existing);

/*
    Validate that input is a directory.
*/
static evp_result validate_directory(const std::filesystem::path& input);

static std::array<unsigned char, 16> compute_md5(const void* ptr, size_t size);

static void serialize_file_desc(const file_desc& file_desc, std::vector<uint8_t>& buffer);

namespace libevp {
    class evp_impl {
    public:
        static evp_result pack_impl(const std::filesystem::path& input_dir, const std::filesystem::path& evp,
            evp_filter filter = evp_filter::none, evp_context* context = nullptr);

        static evp_result unpack_impl(const std::filesystem::path& evp, const std::filesystem::path& output_dir,
            evp_context* context = nullptr);
    };
}

///////////////////////////////////////////////////////////////////////////////
// PUBLIC

evp_result evp::pack(const dir_path_t& input_dir, const file_path_t& evp, evp_filter filter) {
    return evp_impl::pack_impl(input_dir, evp, filter, nullptr);
}

evp_result evp::unpack(const file_path_t& evp, const dir_path_t& output_dir) {
    return evp_impl::unpack_impl(evp, output_dir, nullptr);
}

void evp::pack_async(const dir_path_t& input_dir, const file_path_t& evp, evp_filter filter, evp_context* context) {
    std::thread t([input_dir, evp, filter, context] {
        evp_impl::pack_impl(input_dir, evp, filter, context);
    });
    t.detach();
}

void evp::unpack_async(const file_path_t& evp, const dir_path_t& output_dir, evp_context* context) {
    std::thread t([evp, output_dir, context] {
        evp_impl::unpack_impl(evp, output_dir, context);
    });
    t.detach();
}

evp_result evp::get_files(const file_path_t& evp, std::vector<evp_fd>& files) {
    evp_result result, res;
    result.status = evp_result_status::error;

    res = validate_evp_archive(evp, true);
    if (!res) {
        result.message = res.message;
        return result;
    }

    stream_read stream(evp);
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

    result.status = evp_result_status::ok;
    return result;
}

evp_result evp::get_file(const file_path_t& evp, const evp_fd& fd, std::vector<uint8_t>& buffer) {
    evp_result result, res;
    result.status = evp_result_status::error;

    res = validate_evp_archive(evp, true);
    if (!res) {
        result.message = res.message;
        return result;
    }

    stream_read stream(evp.string());
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

    result.status = evp_result_status::ok;
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
    result.status = evp_result_status::error;
    
    res = validate_evp_archive(evp, true);
    if (!res) {
        result.message = res.message;
        return result;
    }

    stream_read stream(evp.string());
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
            result.status = evp_result_status::ok;
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

evp_result read_structure(stream_read& stream, libevp::format::format::ptr_t& format) {
    evp_result result;
    result.status = evp_result_status::error;

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

    result.status = evp_result_status::ok;
    return result;
}

evp_result determine_format(stream_read& stream, std::shared_ptr<libevp::format::format>& format) {
    evp_result result;
    result.status = evp_result_status::error;

    std::shared_ptr<libevp::format::format> supported_formats[] = {
        static_pointer_cast<libevp::format::format>(std::make_shared<libevp::format::v1::format>())
    };

    for (std::shared_ptr<libevp::format::format> supported_format : supported_formats) {
        if (!supported_format) continue;
        
        supported_format->read_format_desc(stream);
        
        if (supported_format->is_valid) {
            format = supported_format;

            result.status = evp_result_status::ok;
            return result;
        }
    }

    return result;
}

evp_result validate_evp_archive(const std::filesystem::path& input, bool existing) {
    evp_result result;
    result.status = evp_result_status::error;

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

    result.status = evp_result_status::ok;
    return result;
}

evp_result validate_directory(const std::filesystem::path& input) {
    evp_result result;
    result.status = evp_result_status::error;

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

    result.status = evp_result_status::ok;
    return result;
}

std::array<unsigned char, 16> compute_md5(const void* ptr, size_t size) {
    std::array<unsigned char, 16> result{};

    MD5 md5_digest;
    md5_digest.add(ptr, size);
    md5_digest.getHash(result.data());

    return result;
}

void serialize_file_desc(const file_desc& file_desc, std::vector<unsigned char>& buffer) {
    buffer.insert(buffer.end(), (unsigned char*)&(file_desc.path_size), (unsigned char*)&(file_desc.path_size) + sizeof(uint32_t));
    buffer.insert(buffer.end(), file_desc.path.begin(), file_desc.path.end());
    buffer.insert(buffer.end(), (unsigned char*)&(file_desc.data_start_offset), (unsigned char*)&(file_desc.data_start_offset) + sizeof(uint32_t));
    buffer.insert(buffer.end(), (unsigned char*)&(file_desc.data_size), (unsigned char*)&(file_desc.data_size) + sizeof(uint32_t));
    buffer.insert(buffer.end(), (unsigned char*)&(file_desc.data_size), (unsigned char*)&(file_desc.data_size) + sizeof(uint32_t));
    buffer.insert(buffer.end(), 1, 1);
    buffer.insert(buffer.end(), 11, 0);
    buffer.insert(buffer.end(), file_desc.data_hash.data(), file_desc.data_hash.data() + file_desc.data_hash.size());
}

evp_result evp_impl::pack_impl(const std::filesystem::path& input_dir, const std::filesystem::path& evp,
    evp_filter filter, evp_context* context)
{
    evp_result result;
    evp_result res;
    result.status = evp_result_status::error;

    ///////////////////////////////////////////////////////////////////////////
    // VERIFY

    std::filesystem::path input  = input_dir;
    std::filesystem::path output = evp;

    if (!input.is_absolute())
        input = std::filesystem::absolute(input);

    if (!output.is_absolute())
        output = std::filesystem::absolute(output);

    res = verify_pack_paths(input, output);
    if (!res) {
        result.message = res.message;

        if (context)
            context->invoke_finish(result);

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////
    // PACK

    std::vector<file_desc> input_files;
    uint32_t curr_data_offset = v1::DATA_START_OFFSET;
    uint32_t footer_size      = 0;

    auto files = filtering::get_filtered_paths(input, filter);

    float prog_change_x = 90.0f / files.size();
    float prog_change_y = 10.0f / files.size();

    if (context)
        context->invoke_start();

    std::ofstream fout;
    fout.open(output, std::ios::binary);

    if (!fout.is_open()) {
        result.message = "Failed to open .evp file.";

        if (context)
            context->invoke_finish(result);

        return result;
    }

    fout.write(v1::format_desc::HEADER,   sizeof(v1::format_desc::HEADER));
    fout.write(v1::format_desc::RESERVED, sizeof(v1::format_desc::RESERVED));

    for (auto& filename : files) {
        if (context && context->invoke_cancel()) {
            result.status = evp_result_status::cancelled;

            if (context)
                context->invoke_finish(result);

            return result;
        }

        file_desc input_file;

        // save file path
        input_file.path = filename.generic_string();

        // get file content
        std::ifstream input_stream(filename, std::ios::binary);

        if (!input_stream.is_open()) {
            result.message = "Failed to open `" + input_file.path + "`.";

            if (context)
                context->invoke_finish(result);

            return result;
        }

        input_file.data = std::vector<unsigned char>((std::istreambuf_iterator<char>(input_stream)), (std::istreambuf_iterator<char>()));
        input_stream.close();

        // save content size
        input_file.data_size = (uint32_t)input_file.data.size();

        // hash file content
        input_file.data_hash = compute_md5(input_file.data.data(), input_file.data_size);

        fout.write((char*)input_file.data.data(), input_file.data_size);

        // free data
        input_file.data.clear();
        input_file.data.shrink_to_fit();

        // set offset
        input_file.data_start_offset = curr_data_offset;

        input_files.push_back(input_file);

        curr_data_offset += input_file.data_size;

        if (context)
            context->invoke_update(prog_change_x);
    }

    fout.write(v1::format_desc::RESERVED, sizeof(v1::format_desc::RESERVED));

    for (auto& input_file : input_files) {
        if (context && context->invoke_cancel()) {
            result.status = evp_result_status::cancelled;

            if (context)
                context->invoke_finish(result);

            return result;
        }

        // get path relative to input path
        size_t start_index = input_file.path.find(input.generic_string());
        input_file.path.erase(start_index, input.generic_string().size());

        // swap slash directions
        std::replace(input_file.path.begin(), input_file.path.end(), '/', '\\');

        // remove leading slash
        if (input_file.path[0] == '\\')
            input_file.path.erase(0, 1);

        // save path size
        input_file.path_size = (uint32_t)input_file.path.size();

        // pack file desc
        std::vector<uint8_t> file_desc_bytes;
        serialize_file_desc(input_file, file_desc_bytes);

        fout.write((char*)file_desc_bytes.data(), file_desc_bytes.size());

        footer_size += (uint32_t)file_desc_bytes.size();

        if (context)
            context->invoke_update(prog_change_y);
    }

    uint64_t num_files = input_files.size();

    fout.seekp(v1::HEADER_END_OFFSET, std::ios_base::beg);

    fout.write((char*)&curr_data_offset, sizeof(uint32_t));
    fout.write((char*)&footer_size,      sizeof(uint32_t));
    fout.write((char*)&num_files,        sizeof(uint64_t));
    fout.close();

    result.status = evp_result_status::ok;

    if (context)
        context->invoke_finish(result);

    return result;
}

evp_result evp_impl::unpack_impl(const std::filesystem::path& evp, const std::filesystem::path& output_dir,
    evp_context* context)
{
    evp_result result, res;
    result.status = evp_result_status::error;

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

        if (context)
            context->invoke_finish(result);

        return result;
    }

    res = validate_directory(output_dir);
    if (!res) {
        result.message = res.message;

        if (context)
            context->invoke_finish(result);

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////
    // UNPACK

    stream_read stream(input.string());
    if (!stream.is_valid()) {
        result.message = "Failed to open .evp file.";

        if (context)
            context->invoke_finish(result);

        return result;
    }

    format::format::ptr_t format;
    res = read_structure(stream, format);
    if (!res) {
        result.message = res.message;

        if (context)
            context->invoke_finish(result);

        return result;
    }

    float prog_change = 100.0f / format->file_count;

    if (context)
        context->invoke_start();

    try {
        std::vector<uint8_t> buffer{};
        buffer.resize(EVP_BUFFER_SIZE);

        for (evp_fd& fd : format->desc_block->files) {
            if (context && context->invoke_cancel()) {
                result.status = evp_result_status::cancelled;

                if (context)
                    context->invoke_finish(result);

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

            stream_write out_stream(file_path);
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

            if (context)
                context->invoke_update(prog_change);
        }
    }
    catch (const std::exception& e) {
        result.message = e.what();

        if (context)
            context->invoke_finish(result);

        return result;
    }

    result.status = evp_result_status::ok;

    if (context)
        context->invoke_finish(result);

    return result;
}
