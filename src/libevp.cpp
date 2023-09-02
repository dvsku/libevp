#include "libevp.h"

#include <exception>
#include <fstream>
#include <thread>

#include "utilities/filtering.hpp"
#include "versions/formats.hpp"
#include "lib/libdvsku_crypt/libdvsku_crypt.hpp"

using namespace libevp;
using namespace libdvsku;
using namespace libdvsku::crypt;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	#if !defined(NDEBUG) 
		#pragma comment(lib, "libdvsku_crypt/libdvsku_crypt_debug")
	#else 
		#pragma comment(lib, "libdvsku_crypt/libdvsku_crypt_release")
	#endif
#elif defined(__linux__) || defined(__unix__)
	#if !defined(NDEBUG) 
		#error Missing libdvsku_crypt lib
	#else 
		#error Missing libdvsku_crypt lib
	#endif
#else
	#error Unsupported platform
#endif

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

///////////////////////////////////////////////////////////////////////////////
// IMPL FORWARD DECLARE
///////////////////////////////////////////////////////////////////////////////

dv_result pack_impl(const evp::FOLDER_PATH& input, const evp::FILE_PATH& output, bool encrypt = false, const std::string& key = "",
	file_filter filter = file_filter::none, const bool* cancel = nullptr, evp::notify_start started = nullptr,
	evp::notify_update update = nullptr, evp::notify_finish finished = nullptr, evp::notify_error error = nullptr);

dv_result unpack_impl(const evp::FILE_PATH& input, const evp::FOLDER_PATH& output, bool decrypt = false, const std::string& key = "",
	const bool* cancel = nullptr, evp::notify_start started = nullptr, evp::notify_update update = nullptr, 
	evp::notify_finish finished = nullptr, evp::notify_error error = nullptr);

void serialize_file_desc(const file_desc& file_desc, std::vector<unsigned char>& buffer);

dv_result is_evp_header_valid(const evp::FILE_PATH& input);

dv_result are_pack_paths_valid(const evp::FOLDER_PATH& input, const evp::FILE_PATH& output);

dv_result are_unpack_paths_valid(const evp::FILE_PATH& input, const evp::FOLDER_PATH& output);

///////////////////////////////////////////////////////////////////////////////
// PUBLIC
///////////////////////////////////////////////////////////////////////////////

dv_result evp::pack(const FOLDER_PATH& input, const FILE_PATH& output, bool encrypt,
	const std::string& key, file_filter filter)
{
	auto result = are_pack_paths_valid(input, output);
	if (!result) return result;

	FILE_PATH input_path(input);
	FILE_PATH output_path(output);

	if (!input_path.is_absolute())
		input_path = std::filesystem::absolute(input);

	if (!output_path.is_absolute())
		output_path = std::filesystem::absolute(output);

	return pack_impl(input_path, output_path, encrypt, key, filter);
}

dv_result evp::unpack(const FILE_PATH& input, const FOLDER_PATH& output, bool decrypt,
	const std::string& key)
{
	auto result = are_unpack_paths_valid(input, output);
	if (!result) return result;
	
	result = is_evp_header_valid(input);
	if (!result) return result;

	FILE_PATH input_path(input);
	FILE_PATH output_path(output);

	if (!input_path.is_absolute())
		input_path = std::filesystem::absolute(input);

	if (!output_path.is_absolute())
		output_path = std::filesystem::absolute(output);

	return unpack_impl(input_path, output_path, decrypt, key);
}

void evp::pack_async(const FOLDER_PATH& input, const FILE_PATH& output, bool encrypt, 
	const std::string& key, file_filter filter, const bool* cancel, notify_start started, notify_update update, 
	notify_finish finished, notify_error error)
{
	std::thread t([input, output, cancel, encrypt, key, filter, started, update, finished, error] {
		auto result = are_pack_paths_valid(input, output);

		if (!result) {
			if (error) 
				error(result);

			return;
		}

		FILE_PATH input_path(input);
		FILE_PATH output_path(output);

		if (!input_path.is_absolute())
			input_path = std::filesystem::absolute(input);

		if (!output_path.is_absolute())
			output_path = std::filesystem::absolute(output);

		pack_impl(input_path, output_path, encrypt, key, filter, cancel, started, update, finished, error);
	});
	t.detach();
}

void evp::unpack_async(const FILE_PATH& input, const FOLDER_PATH& output, bool decrypt,
	const std::string& key, const bool* cancel, notify_start started, notify_update update,
	notify_finish finished, notify_error error)
{
	std::thread t([input, output, cancel, decrypt, key, started, update, finished, error] {
		auto result = are_unpack_paths_valid(input, output);

		if (!result) {
			if (error) 
				error(result);

			return;
		}

		result = is_evp_header_valid(input);

		if (result) {
			if (error) 
				error(result);

			return;
		}

		FILE_PATH input_path(input);
		FILE_PATH output_path(output);

		if (!input_path.is_absolute())
			input_path = std::filesystem::absolute(input);

		if (!output_path.is_absolute())
			output_path = std::filesystem::absolute(output);

		unpack_impl(input_path, output_path, decrypt, key, cancel, started, update, finished, error);
	});
	t.detach();
}

std::vector<evp::FILE_PATH> evp::get_file_list(const FILE_PATH& input, bool decrypt, const std::string& key) {
	return std::vector<FILE_PATH>();
}

bool evp::is_encrypted(const FILE_PATH& input) {
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// IMPL
///////////////////////////////////////////////////////////////////////////////

dv_result pack_impl(const evp::FOLDER_PATH& input, const evp::FILE_PATH& output, bool encrypt, const std::string& key,
	file_filter filter, const bool* cancel, evp::notify_start started, evp::notify_update update, evp::notify_finish finished, 
	evp::notify_error error) 
{
	std::vector<file_desc> input_files;
	size_t curr_data_offset = v1::DATA_START_OFFSET;
	size_t footer_size = 0;

	libdvsku_crypt crypt(key.c_str());

	auto files = filtering::get_filtered_paths(input, filter);

	float prog_change_x = 90.0f / files.size();
	float prog_change_y = 10.0f / files.size();

	if (started)
		started();

	std::ofstream fout;
	fout.open(output, std::ios::binary);

	fout.write(v1::format_desc::HEADER,	sizeof(v1::format_desc::HEADER));
	fout.write(v1::format_desc::RESERVED, sizeof(v1::format_desc::RESERVED));

	for (evp::FILE_PATH file : files) {
		if (cancel && *cancel) {
			if (finished)
				finished(dv_result(dv_status::cancelled));

			return dv_result(dv_status::cancelled);
		}

		file_desc input_file;

		// save file path
		input_file.m_path = file.generic_string();

		// get file content
		std::ifstream input_stream(file, std::ios::binary);
		input_file.m_data = std::vector<unsigned char>((std::istreambuf_iterator<char>(input_stream)), (std::istreambuf_iterator<char>()));
		input_stream.close();

		if (encrypt && filtering::should_be_encrypted(file))
			crypt.encrypt_buffer(input_file.m_data);

		// save content size
		input_file.m_data_size = (unsigned int)input_file.m_data.size();

		// hash file content
		input_file.m_data_hash = crypt.compute_md5(input_file.m_data.data(), input_file.m_data_size);

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
				finished(dv_result(dv_status::cancelled));

			return dv_result(dv_status::cancelled);
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
		finished(dv_result());

	fout.close();

	return dv_result();
}

dv_result unpack_impl(const evp::FILE_PATH& input, const evp::FOLDER_PATH& output, bool decrypt, const std::string& key,
	const bool* cancel, evp::notify_start started, evp::notify_update update, evp::notify_finish finished, evp::notify_error error) 
{
	size_t data_block_end = 0;
	size_t names_block_size = 0;
	uint64_t file_count = 0;

	libdvsku_crypt crypt(key.c_str());

	std::ifstream fin;
	fin.open(input, std::ios::binary);

	if (!fin.is_open()) {
		dv_result result(dv_status::error, "Could not open input file");
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

	for (int i = 0; i < file_count; i++) {
		if (cancel && *cancel) {
			if (finished)
				finished(dv_result(dv_status::cancelled));

			return dv_result(dv_status::cancelled);
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

		if (decrypt)
			crypt.decrypt_buffer(output_file.m_data);

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
			dv_result result(dv_status::error, "Could not write file : " + output_file.m_path);
			if (error) error(result);
			return result;
		}

		fout.write((char*)output_file.m_data.data(), output_file.m_data.size());
		fout.close();

		if (update)
			update(prog_change);
	}

	if (finished)
		finished(dv_result());

	return dv_result();
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

dv_result is_evp_header_valid(const evp::FILE_PATH& input) {
	std::ifstream fin(input, std::ios::binary);

	if (!fin.is_open())
		return dv_result(dv_status::error, "Could not open input file.");

	char header_buffer[60];
	fin.read(header_buffer, v1::HEADER_END_OFFSET);
	fin.close();

	for (size_t i = 0; i < strlen(header_buffer); i++) {
		if (v1::format_desc::HEADER[i] != header_buffer[i])
			return dv_result(dv_status::error, "Input not an .evp file or .evp version unsupported.");
	}

	return dv_result();
}

dv_result are_pack_paths_valid(const evp::FOLDER_PATH& input, const evp::FILE_PATH& output) {
	evp::FILE_PATH input_path(input);
	evp::FILE_PATH output_path(output);

	try {
		if (!input_path.is_absolute())
			input_path = std::filesystem::absolute(input);

		if (!output_path.is_absolute())
			output_path = std::filesystem::absolute(output);

		if (!std::filesystem::exists(input_path))
			return dv_result(dv_status::error, "Input directory doesn't exist");

		if (!std::filesystem::is_directory(input_path))
			return dv_result(dv_status::error, "Input has to be a directory");

		if (std::filesystem::is_directory(output_path))
			return dv_result(dv_status::error, "Output cannot be a directory");

		if (!output_path.has_filename())
			return dv_result(dv_status::error, "Output must be a file with .evp extension");

		if (!output_path.has_extension() || output_path.extension() != ".evp")
			return dv_result(dv_status::error, "Output extension must be .evp");
	}
	catch (const std::exception& ex) {
		return dv_result(dv_status::error, ex.what());
	}
	catch (...) {
		return dv_result(dv_status::error, "Unknow error occurred.");
	}

	return dv_result();
}

dv_result are_unpack_paths_valid(const evp::FILE_PATH& input, const evp::FOLDER_PATH& output) {
	evp::FILE_PATH input_path(input);
	evp::FILE_PATH output_path(output);

	try {
		if (!input_path.is_absolute())
			input_path = std::filesystem::absolute(input);

		if (!output_path.is_absolute())
			output_path = std::filesystem::absolute(output);

		if (!std::filesystem::exists(output_path))
			return dv_result(dv_status::error, "Output directory doesn't exist");

		if (!std::filesystem::is_directory(output_path))
			return dv_result(dv_status::error, "Output has to be a directory");

		if (std::filesystem::is_directory(input_path))
			return dv_result(dv_status::error, "Input cannot be a directory");

		if (!input_path.has_filename())
			return dv_result(dv_status::error, "Input must be a file with .evp extension");

		if (!input_path.has_extension() || input_path.extension() != ".evp")
			return dv_result(dv_status::error, "Input extension must be .evp");
	}
	catch (const std::exception& ex) {
		return dv_result(dv_status::error, ex.what());
	}
	catch (...) {
		return dv_result(dv_status::error, "Unknow error occurred.");
	}

	return dv_result();
}
