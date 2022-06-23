#pragma once

#ifndef EVP_CORE_CONSTS_H
#define EVP_CORE_CONSTS_H

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING 1;
#include <experimental/filesystem>

namespace filesys = std::experimental::filesystem;

constexpr char UNKNOWN_HEADER_BYTES[60]{ 53,50,53,99,49,55,97,54,97,55,99,102,98,99,100,55,53,52,49,50,101,99,100,48,54,57,100,52,98,55,50,99,51,56,57,0,16,0,0,0,78,79,82,77,65,76,95,80,65,67,75,95,84,89,80,69,100,0,0,0 };
constexpr char RESERVED_BYTES[16]{};

#define DATA_START_OFFSET 76
#define HEADER_END_OFFSET 60
#define OFFSET_BETWEEN_NAMES 36

struct file_desc {
	std::string m_path;
	std::vector<unsigned char> m_data;
	unsigned char m_data_hash[16];
	size_t m_data_size;
	size_t m_path_size;
	size_t m_data_start_offset;
};

static std::vector<filesys::path> get_all_files(filesys::path dir) {
	std::vector<filesys::path> results;

	for (auto const& dir_entry : filesys::recursive_directory_iterator(dir)) {
		if (filesys::is_regular_file(dir_entry.path()))
			results.push_back(dir_entry.path());
	}

	return results;
}

static bool check_header(const char* header) {
	// TODO
}

#endif
