#pragma once

#ifndef EVP_CORE_UNPACKER_H
#define EVP_CORE_UNPACKER_H

#include <string>
#include <algorithm>

#include "consts.h"

namespace evp {
	namespace core {
		static void unpack(filesys::path input, filesys::path output) {
			size_t data_block_end = 0;
			size_t names_block_size = 0;
			uint64_t file_count = 0;

			std::ifstream fin;
			fin.open(input, std::ios::binary);

			if(!fin.is_open())
				throw std::runtime_error("Could not open input file");

			char header_buffer[60];
			fin.read(header_buffer, HEADER_END_OFFSET);

			if(!is_header_valid(header_buffer))
				throw std::runtime_error("Unsupported file type");

			fin.seekg(HEADER_END_OFFSET, std::ios_base::beg);

			fin.read((char*)&data_block_end, sizeof(size_t));
			fin.read((char*)&names_block_size, sizeof(size_t));
			fin.read((char*)&file_count, sizeof(uint64_t));

			float prog_change = 100.0 / file_count;

			size_t curr_name_block_offset = data_block_end + 16;
			size_t curr_data_block_offset = DATA_START_OFFSET;

			draw_progress_bar();

			for (int i = 0; i < file_count; i++) {
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

				curr_name_block_offset += OFFSET_BETWEEN_NAMES;
				curr_data_block_offset += output_file.m_data_size;

				filesys::path dir_path(output);
				dir_path /= output_file.m_path;
				dir_path.remove_filename();
				
				filesys::path full_path(output);
				full_path /= output_file.m_path;

				if (!filesys::is_directory(dir_path)) {
					filesys::create_directories(dir_path);
					filesys::permissions(dir_path, std::experimental::filesystem::perms::all);
				}
				
				std::ofstream fout;
				fout.open(full_path, std::ios::binary);
				
				if (!fout.is_open())
					throw std::runtime_error("Could not write file : " + output_file.m_path);

				fout.write((char*)output_file.m_data.data(), output_file.m_data_size);
				fout.close();

				update_progress(prog_change);
			}

			update_progress(100, false);
		}
	}
}

#endif
