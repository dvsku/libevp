#include "libevp.hpp"

#include "md5/md5.hpp"

#include <exception>
#include <fstream>
#include <thread>
#include <iterator>
#include <array>

#include "utilities/filtering.hpp"
#include "versions/formats.hpp"

using namespace libevp;

///////////////////////////////////////////////////////////////////////////////
// UTILITIES
///////////////////////////////////////////////////////////////////////////////

struct file_desc {
    std::string m_path{};
    std::vector<uint8_t> m_data{};
    std::array<uint8_t, 16> m_data_hash{};
    size_t m_data_size = 0;
    size_t m_path_size = 0;
    size_t m_data_start_offset = 0;
};

libevp::evp::buffer_process_fn buffer_process = nullptr;

///////////////////////////////////////////////////////////////////////////////
// IMPL FORWARD DECLARE
///////////////////////////////////////////////////////////////////////////////

static evp_result pack_impl(const evp::DIR_PATH& input, const evp::FILE_PATH& output, file_filter filter = file_filter::none,
    const bool* cancel = nullptr, evp::notify_start started = nullptr, evp::notify_update update = nullptr, 
    evp::notify_finish finished = nullptr, evp::notify_error error = nullptr);

static evp_result unpack_impl(const evp::FILE_PATH& input, const evp::DIR_PATH& output, const bool* cancel = nullptr,
    evp::notify_start started = nullptr, evp::notify_update update = nullptr, 
    evp::notify_finish finished = nullptr, evp::notify_error error = nullptr);

static std::array<unsigned char, 16> compute_md5(const void* ptr, size_t size);

static void serialize_file_desc(const file_desc& file_desc, std::vector<unsigned char>& buffer);

static evp_result is_evp_header_valid(const evp::FILE_PATH& input);

static evp_result are_pack_paths_valid(const evp::DIR_PATH& input, const evp::FILE_PATH& output);

static evp_result are_unpack_paths_valid(const evp::FILE_PATH& input, const evp::DIR_PATH& output);

///////////////////////////////////////////////////////////////////////////////
// PUBLIC
///////////////////////////////////////////////////////////////////////////////

evp_result evp::pack(const DIR_PATH& input_dir, const FILE_PATH& evp, file_filter filter)
{
    auto result = are_pack_paths_valid(input_dir, evp);
    if (!result) return result;

    FILE_PATH input_path(input_dir);
    FILE_PATH output_path(evp);

    if (!input_path.is_absolute())
        input_path = std::filesystem::absolute(input_dir);

    if (!output_path.is_absolute())
        output_path = std::filesystem::absolute(evp);

    auto res = pack_impl(input_path, output_path, filter);
    buffer_process = nullptr;

    return res;
}

evp_result evp::unpack(const FILE_PATH& evp, const DIR_PATH& output_dir)
{
    auto result = are_unpack_paths_valid(evp, output_dir);
    if (!result) return result;
    
    result = is_evp_header_valid(evp);
    if (!result) return result;

    FILE_PATH input_path(evp);
    FILE_PATH output_path(output_dir);

    if (!input_path.is_absolute())
        input_path = std::filesystem::absolute(evp);

    if (!output_path.is_absolute())
        output_path = std::filesystem::absolute(output_dir);

    auto res = unpack_impl(input_path, output_path);
    buffer_process = nullptr;

    return res;
}

void evp::pack_async(const DIR_PATH& input_dir, const FILE_PATH& evp, file_filter filter, 
    const bool* cancel, notify_start started, notify_update update, 
    notify_finish finished, notify_error error)
{
    std::thread t([input_dir, evp, cancel, filter, started, update, finished, error] {
        auto result = are_pack_paths_valid(input_dir, evp);

        if (!result) {
            if (error) 
                error(result);

            return;
        }

        FILE_PATH input_path(input_dir);
        FILE_PATH output_path(evp);

        if (!input_path.is_absolute())
            input_path = std::filesystem::absolute(input_dir);

        if (!output_path.is_absolute())
            output_path = std::filesystem::absolute(evp);

        pack_impl(input_path, output_path, filter, cancel, started, update, finished, error);

        buffer_process = nullptr;
    });
    t.detach();
}

void evp::unpack_async(const FILE_PATH& evp, const DIR_PATH& output_dir, const bool* cancel, 
    notify_start started, notify_update update, notify_finish finished, notify_error error)
{
    std::thread t([evp, output_dir, cancel, started, update, finished, error] {
        auto result = are_unpack_paths_valid(evp, output_dir);

        if (!result) {
            if (error) 
                error(result);

            return;
        }

        result = is_evp_header_valid(evp);

        if (!result) {
            if (error) 
                error(result);

            return;
        }

        FILE_PATH input_path(evp);
        FILE_PATH output_path(output_dir);

        if (!input_path.is_absolute())
            input_path = std::filesystem::absolute(evp);

        if (!output_path.is_absolute())
            output_path = std::filesystem::absolute(output_dir);

        unpack_impl(input_path, output_path, cancel, started, update, finished, error);

        buffer_process = nullptr;
    });
    t.detach();
}

std::vector<evp::FILE_PATH> evp::get_evp_file_list(const FILE_PATH& evp) {
    std::vector<FILE_PATH> results;

    std::ifstream fin;
    fin.open(evp, std::ios::binary);

    if (!fin.is_open() || fin.bad())
        return results;

    size_t data_block_end    = 0;
    size_t names_block_size = 0;
    uint64_t file_count        = 0;

    fin.seekg(v1::HEADER_END_OFFSET, std::ios_base::beg);

    fin.read((char*)&data_block_end, sizeof(size_t));
    fin.read((char*)&names_block_size, sizeof(size_t));
    fin.read((char*)&file_count, sizeof(uint64_t));

    size_t curr_name_block_offset = data_block_end + 16;

    for (uint64_t i = 0; i < file_count; i++) {
        size_t path_size = 0;
        std::string path = "";

        // go to curr name block pos
        fin.seekg(curr_name_block_offset, std::ios_base::beg);

        // get path size
        fin.read((char*)&path_size, sizeof(size_t));
        path.resize(path_size);

        curr_name_block_offset += sizeof(size_t);

        // get file path
        fin.read(&path[0], path_size);
        curr_name_block_offset += path_size + sizeof(size_t);

        std::replace(path.begin(), path.end(), '\\', '/');

        results.push_back(evp::FILE_PATH(path));

        curr_name_block_offset += v1::GAP_BETWEEN_FILE_DESC;
    }

    fin.close();

    return results;
}

evp_result evp::get_file_from_evp(const FILE_PATH& evp, const FILE_PATH& file, BUFFER& buffer)
{
    std::ifstream fin;
    fin.open(evp, std::ios::binary);
    
    if (!fin.is_open() || fin.bad())
        return evp_result(evp_result::e_status::error, "Could not open evp file");

    size_t data_block_end    = 0;
    size_t names_block_size = 0;
    uint64_t file_count        = 0;

    fin.seekg(v1::HEADER_END_OFFSET, std::ios_base::beg);

    fin.read((char*)&data_block_end, sizeof(size_t));
    fin.read((char*)&names_block_size, sizeof(size_t));
    fin.read((char*)&file_count, sizeof(uint64_t));

    size_t curr_name_block_offset = data_block_end + 16;

    bool found = false;
    for (uint64_t i = 0; i < file_count; i++) {
        file_desc current_file;

        // go to curr name block pos
        fin.seekg(curr_name_block_offset, std::ios_base::beg);

        // get path size
        fin.read((char*)&current_file.m_path_size, sizeof(size_t));
        current_file.m_path.resize(current_file.m_path_size);

        curr_name_block_offset += sizeof(size_t);

        // get file path
        fin.read(&current_file.m_path[0], current_file.m_path_size);
        curr_name_block_offset += current_file.m_path_size;

        // get file data start offset
        fin.read((char*)&current_file.m_data_start_offset, sizeof(size_t));
        curr_name_block_offset += sizeof(size_t);

        std::replace(current_file.m_path.begin(), current_file.m_path.end(), '\\', '/');

        fin.seekg(sizeof(size_t), std::ios_base::cur);

        // get data size
        fin.read((char*)&current_file.m_data_size, sizeof(size_t));
        
        if (current_file.m_path == file.generic_string()) {
            buffer.resize(current_file.m_data_size);

            // go to curr data block pos
            fin.seekg(current_file.m_data_start_offset, std::ios_base::beg);
            fin.read((char*)buffer.data(), current_file.m_data_size);

            if (buffer_process)
                buffer_process(file, buffer);

            found = true;
            break;
        }

        curr_name_block_offset += v1::GAP_BETWEEN_FILE_DESC;
    }

    buffer_process = nullptr;

    return found ? evp_result() : evp_result(evp_result::e_status::error, "File not found.");
}

evp_result evp::get_file_from_evp(const FILE_PATH& evp, const FILE_PATH& file, std::stringstream& stream)
{
    BUFFER buffer;
    
    auto result = get_file_from_evp(evp, file, buffer);
    if (!result)
        return result;

    std::move(buffer.begin(), buffer.end(), std::ostream_iterator<unsigned char>(stream));

    return result;
}

std::vector<evp::FILE_PATH> evp::get_filtered_files(const DIR_PATH& input, file_filter filter) {
    return filtering::get_filtered_paths(input, filter);
}

///////////////////////////////////////////////////////////////////////////////
// IMPL
///////////////////////////////////////////////////////////////////////////////

evp_result pack_impl(const evp::DIR_PATH& input, const evp::FILE_PATH& output, file_filter filter, 
    const bool* cancel, evp::notify_start started, evp::notify_update update, evp::notify_finish finished, evp::notify_error error) 
{
    std::vector<file_desc> input_files;
    size_t curr_data_offset = v1::DATA_START_OFFSET;
    size_t footer_size = 0;

    auto files = filtering::get_filtered_paths(input, filter);

    float prog_change_x = 90.0f / files.size();
    float prog_change_y = 10.0f / files.size();

    if (started)
        started();

    std::ofstream fout;
    fout.open(output, std::ios::binary);

    fout.write(v1::format_desc::HEADER,    sizeof(v1::format_desc::HEADER));
    fout.write(v1::format_desc::RESERVED, sizeof(v1::format_desc::RESERVED));

    for (evp::FILE_PATH file : files) {
        if (cancel && *cancel) {
            if (finished)
                finished(evp_result(evp_result::e_status::cancelled));

            return evp_result(evp_result::e_status::cancelled);
        }

        file_desc input_file;

        // save file path
        input_file.m_path = file.generic_string();

        // get file content
        std::ifstream input_stream(file, std::ios::binary);
        input_file.m_data = std::vector<unsigned char>((std::istreambuf_iterator<char>(input_stream)), (std::istreambuf_iterator<char>()));
        input_stream.close();

        if (buffer_process)
            buffer_process(file, input_file.m_data);

        // save content size
        input_file.m_data_size = (unsigned int)input_file.m_data.size();

        // hash file content
        input_file.m_data_hash = compute_md5(input_file.m_data.data(), input_file.m_data_size);

        fout.write((char*)input_file.m_data.data(), input_file.m_data_size);

        // free data
        input_file.m_data.clear();
        input_file.m_data.shrink_to_fit();

        // set offset
        input_file.m_data_start_offset = curr_data_offset;

        input_files.push_back(input_file);

        curr_data_offset += input_file.m_data_size;

        if (update)
            update(prog_change_x);
    }

    fout.write(v1::format_desc::RESERVED, sizeof(v1::format_desc::RESERVED));

    for (file_desc input_file : input_files) {
        if (cancel && *cancel) {
            if (finished)
                finished(evp_result(evp_result::e_status::cancelled));

            return evp_result(evp_result::e_status::cancelled);
        }

        // get path relative to input path
        int start_index = input_file.m_path.find(input.generic_string());
        input_file.m_path.erase(start_index, input.generic_string().size());

        // swap slash directions
        std::replace(input_file.m_path.begin(), input_file.m_path.end(), '/', '\\');
            
        // remove leading slash
        if (input_file.m_path[0] == '\\')
            input_file.m_path.erase(0, 1);

        // save path size
        input_file.m_path_size = input_file.m_path.size();

        // pack file desc
        std::vector<unsigned char> file_desc_bytes;
        serialize_file_desc(input_file, file_desc_bytes);

        fout.write((char*)file_desc_bytes.data(), file_desc_bytes.size());

        footer_size += file_desc_bytes.size();

        if (update)
            update(prog_change_y);
    }

    uint64_t num_files = input_files.size();

    fout.seekp(v1::HEADER_END_OFFSET, std::ios_base::beg);
    fout.write((char*)&curr_data_offset, sizeof(size_t));
    fout.write((char*)&footer_size, sizeof(size_t));
    fout.write((char*)&num_files, sizeof(uint64_t));

    if (finished)
        finished(evp_result());

    fout.close();

    return evp_result();
}

evp_result unpack_impl(const evp::FILE_PATH& input, const evp::DIR_PATH& output, const bool* cancel, 
    evp::notify_start started, evp::notify_update update, evp::notify_finish finished, evp::notify_error error) 
{
    size_t data_block_end = 0;
    size_t names_block_size = 0;
    uint64_t file_count = 0;

    std::ifstream fin;
    fin.open(input, std::ios::binary);

    if (!fin.is_open()) {
        evp_result result(evp_result::e_status::error, "Could not open input file");
        if (error) error(result);
        return result;
    }

    fin.seekg(v1::HEADER_END_OFFSET, std::ios_base::beg);

    fin.read((char*)&data_block_end, sizeof(size_t));
    fin.read((char*)&names_block_size, sizeof(size_t));
    fin.read((char*)&file_count, sizeof(uint64_t));

    float prog_change = 100.0f / file_count;

    size_t curr_name_block_offset = data_block_end + 16;
    size_t curr_data_block_offset = v1::DATA_START_OFFSET;

    if (started)
        started();

    for (uint64_t i = 0; i < file_count; i++) {
        if (cancel && *cancel) {
            if (finished)
                finished(evp_result(evp_result::e_status::cancelled));

            return evp_result(evp_result::e_status::cancelled);
        }

        file_desc output_file;

        // go to curr name block pos
        fin.seekg(curr_name_block_offset, std::ios_base::beg);

        // get path size
        fin.read((char*)&output_file.m_path_size, sizeof(size_t));
        output_file.m_path.resize(output_file.m_path_size);

        curr_name_block_offset += sizeof(size_t);

        // get file path
        fin.read(&output_file.m_path[0], output_file.m_path_size);
        curr_name_block_offset += output_file.m_path_size + sizeof(size_t);

        std::replace(output_file.m_path.begin(), output_file.m_path.end(), '\\', '/');

        fin.seekg(sizeof(size_t), std::ios_base::cur);

        // get data size
        fin.read((char*)&output_file.m_data_size, sizeof(size_t));
        output_file.m_data.resize(output_file.m_data_size);

        // go to curr data block pos
        fin.seekg(curr_data_block_offset, std::ios_base::beg);
        fin.read((char*)output_file.m_data.data(), output_file.m_data_size);

        if (buffer_process)
            buffer_process(output_file.m_path, output_file.m_data);

        curr_name_block_offset += v1::GAP_BETWEEN_FILE_DESC;
        curr_data_block_offset += output_file.m_data_size;

        evp::FILE_PATH dir_path(output);
        dir_path /= output_file.m_path;
        dir_path.remove_filename();

        evp::FILE_PATH full_path(output);
        full_path /= output_file.m_path;

        if (!std::filesystem::is_directory(dir_path)) {
            std::filesystem::create_directories(dir_path);
            std::filesystem::permissions(dir_path, std::filesystem::perms::all);
        }

        std::ofstream fout;
        fout.open(full_path, std::ios::binary);

        if (!fout.is_open()) {
            evp_result result(evp_result::e_status::error, "Could not write file : " + output_file.m_path);
            if (error) error(result);
            return result;
        }

        fout.write((char*)output_file.m_data.data(), output_file.m_data.size());
        fout.close();

        if (update)
            update(prog_change);
    }

    if (finished)
        finished(evp_result());

    return evp_result();
}

std::array<unsigned char, 16> compute_md5(const void* ptr, size_t size) {
    std::array<unsigned char, 16> result{};

    MD5 md5_digest;
    md5_digest.add(ptr, size);
    md5_digest.getHash(result.data());

    return result;
}

void serialize_file_desc(const file_desc& file_desc, std::vector<unsigned char>& buffer) {
    buffer.insert(buffer.end(), (unsigned char*)&(file_desc.m_path_size), (unsigned char*)&(file_desc.m_path_size) + sizeof(size_t));
    buffer.insert(buffer.end(), file_desc.m_path.begin(), file_desc.m_path.end());
    buffer.insert(buffer.end(), (unsigned char*)&(file_desc.m_data_start_offset), (unsigned char*)&(file_desc.m_data_start_offset) + sizeof(size_t));
    buffer.insert(buffer.end(), (unsigned char*)&(file_desc.m_data_size), (unsigned char*)&(file_desc.m_data_size) + sizeof(size_t));
    buffer.insert(buffer.end(), (unsigned char*)&(file_desc.m_data_size), (unsigned char*)&(file_desc.m_data_size) + sizeof(size_t));
    buffer.insert(buffer.end(), 1, 1);
    buffer.insert(buffer.end(), 11, 0);
    buffer.insert(buffer.end(), file_desc.m_data_hash.data(), file_desc.m_data_hash.data() + file_desc.m_data_hash.size());
}

evp_result is_evp_header_valid(const evp::FILE_PATH& input) {
    std::ifstream fin(input, std::ios::binary);

    if (!fin.is_open())
        return evp_result(evp_result::e_status::error, "Could not open input file.");

    char header_buffer[60];
    fin.read(header_buffer, v1::HEADER_END_OFFSET);
    fin.close();

    for (size_t i = 0; i < 60; i++) {
        if (v1::format_desc::HEADER[i] != header_buffer[i])
            return evp_result(evp_result::e_status::error, "Input not an .evp file or .evp version unsupported.");
    }

    return evp_result();
}

evp_result are_pack_paths_valid(const evp::DIR_PATH& input, const evp::FILE_PATH& output) {
    evp::FILE_PATH input_path(input);
    evp::FILE_PATH output_path(output);

    try {
        if (!input_path.is_absolute())
            input_path = std::filesystem::absolute(input);

        if (!output_path.is_absolute())
            output_path = std::filesystem::absolute(output);

        if (!std::filesystem::exists(input_path))
            return evp_result(evp_result::e_status::error, "Input directory doesn't exist");

        if (!std::filesystem::is_directory(input_path))
            return evp_result(evp_result::e_status::error, "Input has to be a directory");

        if (std::filesystem::is_directory(output_path))
            return evp_result(evp_result::e_status::error, "Output cannot be a directory");

        if (!output_path.has_filename())
            return evp_result(evp_result::e_status::error, "Output must be a file with .evp extension");

        if (!output_path.has_extension() || output_path.extension() != ".evp")
            return evp_result(evp_result::e_status::error, "Output extension must be .evp");
    }
    catch (const std::exception& ex) {
        return evp_result(evp_result::e_status::error, ex.what());
    }
    catch (...) {
        return evp_result(evp_result::e_status::error, "Unknow error occurred.");
    }

    return evp_result();
}

evp_result are_unpack_paths_valid(const evp::FILE_PATH& input, const evp::DIR_PATH& output) {
    evp::FILE_PATH input_path(input);
    evp::FILE_PATH output_path(output);

    try {
        if (!input_path.is_absolute())
            input_path = std::filesystem::absolute(input);

        if (!output_path.is_absolute())
            output_path = std::filesystem::absolute(output);

        if (!std::filesystem::exists(output_path))
            return evp_result(evp_result::e_status::error, "Output directory doesn't exist");

        if (!std::filesystem::is_directory(output_path))
            return evp_result(evp_result::e_status::error, "Output has to be a directory");

        if (std::filesystem::is_directory(input_path))
            return evp_result(evp_result::e_status::error, "Input cannot be a directory");

        if (!input_path.has_filename())
            return evp_result(evp_result::e_status::error, "Input must be a file with .evp extension");

        if (!input_path.has_extension() || input_path.extension() != ".evp")
            return evp_result(evp_result::e_status::error, "Input extension must be .evp");
    }
    catch (const std::exception& ex) {
        return evp_result(evp_result::e_status::error, ex.what());
    }
    catch (...) {
        return evp_result(evp_result::e_status::error, "Unknow error occurred.");
    }

    return evp_result();
}
