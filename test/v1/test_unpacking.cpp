#include "src/libevp.h"
#include "test/utilities_assert.hpp"

#include <fstream>
#include <iterator>
#include <string>
#include <algorithm>
#include <regex>

using namespace libevp;

bool compare_files(const std::string& p1, const std::string& p2) {
	std::ifstream f1(p1, std::ifstream::binary | std::ifstream::ate);
	std::ifstream f2(p2, std::ifstream::binary | std::ifstream::ate);

	if (f1.fail() || f2.fail()) {
		return false;
	}

	if (f1.tellg() != f2.tellg()) {
		return false;
	}

	f1.seekg(0, std::ifstream::beg);
	f2.seekg(0, std::ifstream::beg);
	return std::equal(std::istreambuf_iterator<char>(f1.rdbuf()),
		std::istreambuf_iterator<char>(),
		std::istreambuf_iterator<char>(f2.rdbuf()));
}

void v1_unpacking_single_file(const std::string& base) {
	std::string input	= base + std::string("/test/v1/resources/valid_single_file.evp");
	std::string output	= base + std::string("/test/v1/resources/unpack_here/");
	std::string valid	= base + std::string("/test/v1/resources/files_to_pack/subfolder_2/text_3.txt");

	std::filesystem::create_directories(output);

	std::string output_file = base + std::string("/test/v1/resources/unpack_here/text_3.txt");

	try {
		auto r1 = evp::unpack(input, output);

		if (!r1)
			std::filesystem::remove_all(output);

		ASSERT(r1);

		auto r2 = compare_files(output_file, valid);
		std::filesystem::remove_all(output);

		ASSERT(r2);
	}
	catch (...) { ASSERT(false); }
}

void v1_unpacking_single_file_encrypted(const std::string& base) {
	std::string input	= base + std::string("/test/v1/resources/valid_single_file_encrypted.evp");
	std::string output	= base + std::string("/test/v1/resources/unpack_here/");
	std::string valid	= base + std::string("/test/v1/resources/files_to_pack/subfolder_2/text_3.txt");

	std::filesystem::create_directories(output);

	std::string output_file = base + std::string("/test/v1/resources/unpack_here/text_3.txt");

	try {
		auto r1 = evp::unpack(input, output, true, "dvsku");

		if (!r1)
			std::filesystem::remove_all(output);

		ASSERT(r1);

		auto r2 = compare_files(output_file, valid);
		std::filesystem::remove_all(output);

		ASSERT(r2);
	}
	catch (...) { ASSERT(false); }
}

void v1_unpacking_folder(const std::string& base) {
	std::string input	= base + std::string("/test/v1/resources/valid_folders.evp");
	std::string output	= base + std::string("/test/v1/resources/unpack_here/");
	std::string valid	= base + std::string("/test/v1/resources/files_to_pack/");

	std::filesystem::create_directories(output);

	try {
		auto r1 = evp::unpack(input, output);

		if (!r1)
			std::filesystem::remove_all(output);

		ASSERT(r1);

		for (auto const& dir_entry : std::filesystem::recursive_directory_iterator(output)) {
			if (std::filesystem::is_regular_file(dir_entry.path())) {
				auto relative_path = std::regex_replace(dir_entry.path().generic_string(), std::regex(output), "");
				std::string valid_file = valid + relative_path;

				auto r2 = compare_files(dir_entry.path().generic_string(), valid_file);

				if(!r2)
					std::filesystem::remove_all(output);

				ASSERT(r2);
			}
		}

		std::filesystem::remove_all(output);
	}
	catch (...) { ASSERT(false); }
}

void v1_unpacking_folder_encrypted(const std::string& base) {
	std::string input	= base + std::string("/test/v1/resources/valid_folders_encrypted.evp");
	std::string output	= base + std::string("/test/v1/resources/unpack_here/");
	std::string valid	= base + std::string("/test/v1/resources/files_to_pack/");

	std::filesystem::create_directories(output);

	try {
		auto r1 = evp::unpack(input, output, true, "dvsku");

		if (!r1)
			std::filesystem::remove_all(output);

		ASSERT(r1);

		for (auto const& dir_entry : std::filesystem::recursive_directory_iterator(output)) {
			if (std::filesystem::is_regular_file(dir_entry.path())) {
				auto relative_path = std::regex_replace(dir_entry.path().generic_string(), std::regex(output), "");
				std::string valid_file = valid + relative_path;

				auto r2 = compare_files(dir_entry.path().generic_string(), valid_file);

				if (!r2)
					std::filesystem::remove_all(output);

				ASSERT(r2);
			}
		}

		std::filesystem::remove_all(output);
	}
	catch (...) { ASSERT(false); }
}

int main(int argc, char* argv[]) {
	if (argc == 3) {
		switch (std::stoi(argv[1])) {
			case 0: v1_unpacking_single_file(argv[2]);					break;
			case 1: v1_unpacking_single_file_encrypted(argv[2]);		break;
			case 2: v1_unpacking_folder(argv[2]);						break;
			case 3: v1_unpacking_folder_encrypted(argv[2]);				break;
			default: break;
		}
	}

	return 0;
}