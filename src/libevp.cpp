#include "libevp.h"

#include <exception>
#include <fstream>
#include <thread>

#include "lib/md5/md5.h"
#include "lib/libdvsku_crypt/libdvsku_crypt.h"

dvsku::evp::evp_result dvsku::evp::pack(FOLDER_PATH_REF_C input, FILE_PATH_REF_C output, bool encrypt,
	const std::string& key, FILE_FILTER_REF_C filter)
{
	evp_result result;
	result = are_pack_paths_valid(input, output);

	if (result.m_code != evp_status::ok)
		return result;

	FILE_PATH input_path(input);
	FILE_PATH output_path(output);

	if (!input_path.is_absolute())
		input_path = std_filesys::absolute(input);

	if (!output_path.is_absolute())
		output_path = std_filesys::absolute(output);

	return pack_impl(input_path, output_path, encrypt, key, filter);
}

dvsku::evp::evp_result dvsku::evp::unpack(FILE_PATH_REF_C input, FOLDER_PATH_REF_C output, bool decrypt,
	const std::string& key)
{
	evp_result result;
	result = are_unpack_paths_valid(input, output);

	if (result.m_code != evp_status::ok) 
		return result;
	
	result = is_evp_header_valid(input);

	if (result.m_code != evp_status::ok)
		return result;

	FILE_PATH input_path(input);
	FILE_PATH output_path(output);

	if (!input_path.is_absolute())
		input_path = std_filesys::absolute(input);

	if (!output_path.is_absolute())
		output_path = std_filesys::absolute(output);

	return unpack_impl(input_path, output_path, decrypt, key);
}

void dvsku::evp::pack_async(FOLDER_PATH_REF_C input, FILE_PATH_REF_C output, bool encrypt,
	const std::string& key, FILE_FILTER_REF_C filter, const bool* cancel, notify_start started, notify_update update,
	notify_finish finished, notify_error error)
{
	std::thread t([input, output, cancel, encrypt, key, filter, started, update, finished, error] {
		evp_result result;
		result = are_pack_paths_valid(input, output);

		if (result.m_code != evp_status::ok) {
			if (error) error(result);
			return;
		}

		FILE_PATH input_path(input);
		FILE_PATH output_path(output);

		if (!input_path.is_absolute())
			input_path = std_filesys::absolute(input);

		if (!output_path.is_absolute())
			output_path = std_filesys::absolute(output);

		pack_impl(input_path, output_path, encrypt, key, filter, cancel, started, update, finished, error);
	});
	t.detach();
}

void dvsku::evp::unpack_async(FILE_PATH_REF_C input, FOLDER_PATH_REF_C output, bool decrypt,
	const std::string& key, const bool* cancel, notify_start started, notify_update update,
	notify_finish finished, notify_error error)
{
	std::thread t([input, output, cancel, decrypt, key, started, update, finished, error] {
		evp_result result;
		result = are_unpack_paths_valid(input, output);

		if (result.m_code != evp_status::ok) {
			if (error) error(result);
			return;
		}

		result = is_evp_header_valid(input);

		if (result.m_code != evp_status::ok) {
			if (error) error(result);
			return;
		}

		FILE_PATH input_path(input);
		FILE_PATH output_path(output);

		if (!input_path.is_absolute())
			input_path = std_filesys::absolute(input);

		if (!output_path.is_absolute())
			output_path = std_filesys::absolute(output);

		unpack_impl(input_path, output_path, decrypt, key, cancel, started, update, finished, error);
	});
	t.detach();
}

dvsku::evp::evp_result dvsku::evp::pack_impl(FOLDER_PATH_REF_C input, FILE_PATH_REF_C output, bool encrypt,
	const std::string& key, FILE_FILTER_REF_C filter, const bool* cancel, notify_start started, notify_update update,
	notify_finish finished, notify_error error)
{
	std::vector<file_desc> input_files;
	size_t curr_data_offset = DATA_START_OFFSET;
	size_t footer_size = 0;

	dvsku::crypt::libdvsku_crypt crypt(key.c_str());

	auto files = dvsku::filesys::utilities::get_files_evp(input, filter);

	float prog_change_x = 90.0 / files.size();
	float prog_change_y = 10.0 / files.size();
	
	if (started)
		started();

	std::ofstream fout;
	fout.open(output, std::ios::binary);

	fout.write(EVP_V1_HEADER, sizeof(EVP_V1_HEADER));
	fout.write(RESERVED_BYTES, sizeof(RESERVED_BYTES));

	for (FILE_PATH file : files) {
		if (cancel != nullptr) {
			if (*cancel) {
				if (finished)
					finished(dvsku::evp::evp_status::cancelled);

				return evp_result();
			}
		}

		file_desc input_file;

		// save file path
		input_file.m_path = file.u8string();

		// get file content
		std::ifstream input_stream(file, std::ios::binary);
		input_file.m_data = std::vector<unsigned char>((std::istreambuf_iterator<char>(input_stream)), (std::istreambuf_iterator<char>()));
		input_stream.close();

		if (encrypt)
			crypt.encrypt_buffer(input_file.m_data);

		// save content size
		input_file.m_data_size = (unsigned int)input_file.m_data.size();

		// hash file content
		MD5 md5_digest;
		md5_digest.add(input_file.m_data.data(), input_file.m_data_size);
		md5_digest.getHash(input_file.m_data_hash);

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

	fout.write(RESERVED_BYTES, sizeof(RESERVED_BYTES));

	for (file_desc input_file : input_files) {
		if (cancel != nullptr) {
			if (*cancel) {
				if (finished)
					finished(dvsku::evp::evp_status::cancelled);

				return evp_result();
			}
		}

		// get path relative to input path
		int start_index = input_file.m_path.find(input.u8string());
		input_file.m_path.erase(start_index, input.u8string().size());

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

	fout.seekp(HEADER_END_OFFSET, std::ios_base::beg);
	fout.write((char*)&curr_data_offset, sizeof(size_t));
	fout.write((char*)&footer_size, sizeof(size_t));
	fout.write((char*)&num_files, sizeof(uint64_t));

	if (finished)
		finished(dvsku::evp::evp_status::ok);

	fout.close();

	return evp_result();
}

dvsku::evp::evp_result dvsku::evp::unpack_impl(FILE_PATH_REF_C input, FOLDER_PATH_REF_C output, bool decrypt,
	const std::string& key, const bool* cancel, notify_start started, notify_update update,
	notify_finish finished, notify_error error)
{
	size_t data_block_end = 0;
	size_t names_block_size = 0;
	uint64_t file_count = 0;

	dvsku::crypt::libdvsku_crypt crypt(key.c_str());

	std::ifstream fin;
	fin.open(input, std::ios::binary);

	if (!fin.is_open()) {
		evp_result result(evp_status::error, "Could not open input file");
		if (error) error(result);
		return result;
	}

	fin.seekg(HEADER_END_OFFSET, std::ios_base::beg);

	fin.read((char*)&data_block_end, sizeof(size_t));
	fin.read((char*)&names_block_size, sizeof(size_t));
	fin.read((char*)&file_count, sizeof(uint64_t));

	float prog_change = 100.0 / file_count;

	size_t curr_name_block_offset = data_block_end + 16;
	size_t curr_data_block_offset = DATA_START_OFFSET;

	if (started)
		started();

	for (int i = 0; i < file_count; i++) {
		if (cancel != nullptr) {
			if (*cancel) {
				if (finished)
					finished(dvsku::evp::evp_status::cancelled);

				return evp_result();
			}
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
			crypt.decrypt_and_decompress_buffer(output_file.m_data);

		curr_name_block_offset += OFFSET_BETWEEN_FILE_DESC;
		curr_data_block_offset += output_file.m_data_size;

		FILE_PATH dir_path(output);
		dir_path /= output_file.m_path;
		dir_path.remove_filename();

		FILE_PATH full_path(output);
		full_path /= output_file.m_path;

		if (!std_filesys::is_directory(dir_path)) {
			std_filesys::create_directories(dir_path);
			std_filesys::permissions(dir_path, std::experimental::filesystem::perms::all);
		}

		std::ofstream fout;
		fout.open(full_path, std::ios::binary);

		if (!fout.is_open()) {
			evp_result result(evp_status::error, "Could not write file : " + output_file.m_path);
			if (error) error(result);
			return result;
		}

		fout.write((char*)output_file.m_data.data(), output_file.m_data.size());
		fout.close();

		if (update)
			update(prog_change);
	}

	if (finished)
		finished(dvsku::evp::evp_status::ok);
	
	return evp_result();
}

void dvsku::evp::serialize_file_desc(const file_desc& file_desc, std::vector<unsigned char>& buffer) {
	buffer.insert(buffer.end(), (unsigned char*)&(file_desc.m_path_size), (unsigned char*)&(file_desc.m_path_size) + sizeof(size_t));
	buffer.insert(buffer.end(), file_desc.m_path.begin(), file_desc.m_path.end());
	buffer.insert(buffer.end(), (unsigned char*)&(file_desc.m_data_start_offset), (unsigned char*)&(file_desc.m_data_start_offset) + sizeof(size_t));
	buffer.insert(buffer.end(), (unsigned char*)&(file_desc.m_data_size), (unsigned char*)&(file_desc.m_data_size) + sizeof(size_t));
	buffer.insert(buffer.end(), (unsigned char*)&(file_desc.m_data_size), (unsigned char*)&(file_desc.m_data_size) + sizeof(size_t));
	buffer.insert(buffer.end(), 1, 1);
	buffer.insert(buffer.end(), 11, 0);
	buffer.insert(buffer.end(), file_desc.m_data_hash, file_desc.m_data_hash + sizeof(file_desc.m_data_hash));
}

bool dvsku::evp::is_encrypted(FILE_PATH_REF_C input) {
	return false;
}

dvsku::evp::evp_result dvsku::evp::is_evp_header_valid(FILE_PATH_REF_C input) {
	std::ifstream fin(input, std::ios::binary);
	
	if (!fin.is_open())
		return evp_result(evp_status::error, "Could not open input file.");

	char header_buffer[60];
	fin.read(header_buffer, HEADER_END_OFFSET);
	fin.close();

	for (int i = 0; i < strlen(header_buffer); i++) {
		if (EVP_V1_HEADER[i] != header_buffer[i])
			return evp_result(evp_status::error, "Input not an .evp file or .evp version unsupported.");
	}

	return evp_result();
}

dvsku::evp::evp_result dvsku::evp::are_pack_paths_valid(FOLDER_PATH_REF_C input, FILE_PATH_REF_C output) {
	FILE_PATH input_path(input);
	FILE_PATH output_path(output);

	try {
		if (!input_path.is_absolute())
			input_path = std_filesys::absolute(input);

		if (!output_path.is_absolute())
			output_path = std_filesys::absolute(output);

		if (!std_filesys::exists(input_path))
			return evp_result(evp_status::error, "Input directory doesn't exist");

		if (!std_filesys::is_directory(input_path))
			return evp_result(evp_status::error, "Input has to be a directory");

		if (std_filesys::is_directory(output_path))
			return evp_result(evp_status::error, "Output cannot be a directory");

		if (!output_path.has_filename())
			return evp_result(evp_status::error, "Output must be a file with .evp extension");

		if (!output_path.has_extension() || output_path.extension() != ".evp")
			return evp_result(evp_status::error, "Output extension must be .evp");
	}
	catch (const std::exception& ex) {
		return evp_result(evp_status::error, ex.what());
	}
	catch (...) {
		return evp_result(evp_status::error, "Unknow error occurred.");
	}

	return evp_result();
}

dvsku::evp::evp_result dvsku::evp::are_unpack_paths_valid(FILE_PATH_REF_C input, FOLDER_PATH_REF_C output) {
	FILE_PATH input_path(input);
	FILE_PATH output_path(output);

	try {
		if (!input_path.is_absolute())
			input_path = std_filesys::absolute(input);

		if (!output_path.is_absolute())
			output_path = std_filesys::absolute(output);

		if (!std_filesys::exists(output_path))
			return evp_result(evp_status::error, "Output directory doesn't exist");

		if (!std_filesys::is_directory(output_path))
			return evp_result(evp_status::error, "Output has to be a directory");

		if (std_filesys::is_directory(input_path))
			return evp_result(evp_status::error, "Input cannot be a directory");

		if (!input_path.has_filename())
			return evp_result(evp_status::error, "Input must be a file with .evp extension");

		if (!input_path.has_extension() || input_path.extension() != ".evp")
			return evp_result(evp_status::error, "Input extension must be .evp");
	}
	catch (const std::exception& ex) {
		return evp_result(evp_status::error, ex.what());
	}
	catch (...) {
		return evp_result(evp_status::error, "Unknow error occurred.");
	}

	return evp_result();
}
