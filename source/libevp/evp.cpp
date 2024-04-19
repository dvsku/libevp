#include "libevp/evp.hpp"
#include "libevp/utilities/filtering.hpp"
#include "libevp/versions/formats.hpp"
#include "md5/md5.hpp"

#include <thread>
#include <vector>
#include <array>
#include <fstream>

using namespace libevp;

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

static evp_result verify_pack_paths(const std::filesystem::path& input, const std::filesystem::path& output);
static evp_result verify_unpack_paths(const std::filesystem::path& input, const std::filesystem::path& output);

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

std::vector<evp::file_path_t> evp::get_evp_file_list(const file_path_t& evp) {
    std::vector<evp::file_path_t> results;

    std::ifstream fin;
    fin.open(evp, std::ios::binary);

    if (!fin.is_open())
        return results;

    uint32_t data_block_end   = 0;
    uint32_t names_block_size = 0;
    uint64_t file_count       = 0;

    fin.seekg(v1::HEADER_END_OFFSET, std::ios_base::beg);

    fin.read((char*)&data_block_end,   sizeof(uint32_t));
    fin.read((char*)&names_block_size, sizeof(uint32_t));
    fin.read((char*)&file_count,       sizeof(uint64_t));

    uint32_t curr_name_block_offset = data_block_end + 16;

    for (uint64_t i = 0; i < file_count; i++) {
        uint32_t    path_size = 0;
        std::string path      = "";

        // go to curr name block pos
        fin.seekg(curr_name_block_offset, std::ios_base::beg);

        // get path size
        fin.read((char*)&path_size, sizeof(uint32_t));
        path.resize(path_size);

        curr_name_block_offset += sizeof(uint32_t);

        // get file path
        fin.read(&path[0], path_size);
        curr_name_block_offset += path_size + sizeof(uint32_t);

        std::replace(path.begin(), path.end(), '\\', '/');

        results.push_back(std::filesystem::path(path));

        curr_name_block_offset += v1::GAP_BETWEEN_FILE_DESC;
    }

    fin.close();

    return results;
}

evp_result evp::get_file_from_evp(const file_path_t& evp, const file_path_t& file, std::vector<uint8_t>& buffer, evp_context* context) {
    evp_result result;
    result.status = evp_result_status::error;

    std::ifstream fin;
    fin.open(evp, std::ios::binary);

    if (!fin.is_open()) {
        result.message = "Failed to open evp file.";
        return result;
    }

    uint32_t data_block_end   = 0;
    uint32_t names_block_size = 0;
    uint64_t file_count       = 0;

    fin.seekg(v1::HEADER_END_OFFSET, std::ios_base::beg);

    fin.read((char*)&data_block_end,   sizeof(uint32_t));
    fin.read((char*)&names_block_size, sizeof(uint32_t));
    fin.read((char*)&file_count,       sizeof(uint64_t));

    uint32_t curr_name_block_offset = data_block_end + 16;

    bool found = false;
    for (uint64_t i = 0; i < file_count; i++) {
        file_desc current_file;

        // go to curr name block pos
        fin.seekg(curr_name_block_offset, std::ios_base::beg);

        // get path size
        fin.read((char*)&current_file.path_size, sizeof(uint32_t));
        current_file.path.resize(current_file.path_size);

        curr_name_block_offset += sizeof(uint32_t);

        // get file path
        fin.read(&current_file.path[0], current_file.path_size);
        curr_name_block_offset += current_file.path_size;

        // get file data start offset
        fin.read((char*)&current_file.data_start_offset, sizeof(uint32_t));
        curr_name_block_offset += sizeof(uint32_t);

        std::replace(current_file.path.begin(), current_file.path.end(), '\\', '/');

        fin.seekg(sizeof(uint32_t), std::ios_base::cur);

        // get data size
        fin.read((char*)&current_file.data_size, sizeof(uint32_t));

        if (current_file.path == file.generic_string()) {
            buffer.resize(current_file.data_size);

            // go to curr data block pos
            fin.seekg(current_file.data_start_offset, std::ios_base::beg);
            fin.read((char*)buffer.data(), current_file.data_size);

            if (context)
                context->invoke_buffer_processing(file, buffer);

            found = true;
            break;
        }

        curr_name_block_offset += v1::GAP_BETWEEN_FILE_DESC;
    }

    if (found) {
        result.status = evp_result_status::ok;
    }
    else {
        result.status  = evp_result_status::error;
        result.message = "File not found.";
    }

    return result;
}

evp_result evp::get_file_from_evp(const file_path_t& evp, const file_path_t& file, std::stringstream& stream, evp_context* context) {
    std::vector<uint8_t> buffer;

    auto result = get_file_from_evp(evp, file, buffer);
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

evp_result verify_pack_paths(const std::filesystem::path& input, const std::filesystem::path& output) {
    evp_result result;
    result.status = evp_result_status::error;

    try {
        if (!std::filesystem::exists(input)) {
            result.message = "Input directory doesn't exist.";
            return result;
        }

        if (!std::filesystem::is_directory(input)) {
            result.message = "Input has to be a directory.";
            return result;
        }

        if (std::filesystem::is_directory(output)) {
            result.message = "Output cannot be a directory.";
            return result;
        }

        if (!output.has_filename()) {
            result.message = "Output must be a file with .evp extension.";
            return result;
        }

        if (!output.has_extension() || output.extension() != ".evp") {
            result.message = "Output extension must be .evp.";
            return result;
        }
    }
    catch (const std::exception& ex) {
        result.message = ex.what();
        return result;
    }
    catch (...) {
        result.message = "Critical fail.";
        return result;
    }

    result.status = evp_result_status::ok;
    return result;
}

evp_result verify_unpack_paths(const std::filesystem::path& input, const std::filesystem::path& output) {
    evp_result result;
    result.status = evp_result_status::error;

    try {
        if (!std::filesystem::exists(output)) {
            result.message = "Output directory doesn't exist.";
            return result;
        }

        if (!std::filesystem::is_directory(output)) {
            result.message = "Output has to be a directory.";
            return result;
        }

        if (std::filesystem::is_directory(input)) {
            result.message = "Input cannot be a directory.";
            return result;
        }

        if (!input.has_filename()) {
            result.message = "Input must be a file with .evp extension.";
            return result;
        }

        if (!input.has_extension() || input.extension() != ".evp") {
            result.message = "Input extension must be .evp.";
            return result;
        }
    }
    catch (const std::exception& ex) {
        result.message = ex.what();
        return result;
    }
    catch (...) {
        result.message = "Critical fail.";
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

        if (context)
            context->invoke_buffer_processing(filename, input_file.data);

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
        int start_index = input_file.path.find(input.generic_string());
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

        footer_size += file_desc_bytes.size();

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
    evp_result result;
    evp_result res;
    result.status = evp_result_status::error;

    ///////////////////////////////////////////////////////////////////////////
    // VERIFY

    std::filesystem::path input  = evp;
    std::filesystem::path output = output_dir;

    if (!input.is_absolute())
        input = std::filesystem::absolute(input);

    if (!output.is_absolute())
        output = std::filesystem::absolute(output);

    res = verify_unpack_paths(evp, output_dir);
    if (!res) {
        result.message = res.message;

        if (context)
            context->invoke_finish(result);

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////
    // UNPACK

    uint32_t data_block_end   = 0;
    uint32_t names_block_size = 0;
    uint64_t file_count       = 0;

    std::ifstream fin;
    fin.open(input, std::ios::binary);

    if (!fin.is_open()) {
        result.message = "Failed to open .evp file.";

        if (context)
            context->invoke_finish(result);

        return result;
    }

    fin.seekg(v1::HEADER_END_OFFSET, std::ios_base::beg);

    fin.read((char*)&data_block_end,   sizeof(uint32_t));
    fin.read((char*)&names_block_size, sizeof(uint32_t));
    fin.read((char*)&file_count,       sizeof(uint64_t));

    float prog_change = 100.0f / file_count;

    uint32_t curr_name_block_offset = data_block_end + 16;
    uint32_t curr_data_block_offset = v1::DATA_START_OFFSET;

    if (context)
        context->invoke_start();

    for (uint64_t i = 0; i < file_count; i++) {
        if (context && context->invoke_cancel()) {
            result.status = evp_result_status::cancelled;

            if (context)
                context->invoke_finish(result);

            return result;
        }

        file_desc output_file;

        // go to curr name block pos
        fin.seekg(curr_name_block_offset, std::ios_base::beg);

        // get path size
        fin.read((char*)&output_file.path_size, sizeof(uint32_t));
        output_file.path.resize(output_file.path_size);

        curr_name_block_offset += sizeof(uint32_t);

        // get file path
        fin.read(&output_file.path[0], output_file.path_size);
        curr_name_block_offset += output_file.path_size + sizeof(uint32_t);

        std::replace(output_file.path.begin(), output_file.path.end(), '\\', '/');

        fin.seekg(sizeof(uint32_t), std::ios_base::cur);

        // get data size
        fin.read((char*)&output_file.data_size, sizeof(uint32_t));
        output_file.data.resize(output_file.data_size);

        // go to curr data block pos
        fin.seekg(curr_data_block_offset, std::ios_base::beg);
        fin.read((char*)output_file.data.data(), output_file.data_size);

        if (context)
            context->invoke_buffer_processing(output_file.path, output_file.data);

        curr_name_block_offset += v1::GAP_BETWEEN_FILE_DESC;
        curr_data_block_offset += output_file.data_size;

        std::filesystem::path dir_path(output);
        dir_path /= output_file.path;
        dir_path.remove_filename();

        std::filesystem::path full_path(output);
        full_path /= output_file.path;

        if (!std::filesystem::is_directory(dir_path)) {
            std::filesystem::create_directories(dir_path);
            std::filesystem::permissions(dir_path, std::filesystem::perms::all);
        }

        std::ofstream fout;
        fout.open(full_path, std::ios::binary);

        if (!fout.is_open()) {
            result.message = "Failed to write `" + output_file.path + "` file.";

            if (context)
                context->invoke_finish(result);

            return result;
        }

        fout.write((char*)output_file.data.data(), output_file.data.size());
        fout.close();

        if (context)
            context->invoke_update(prog_change);
    }

    result.status = evp_result_status::ok;

    if (context)
        context->invoke_finish(result);

    return result;
}
