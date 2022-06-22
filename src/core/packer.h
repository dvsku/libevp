#pragma once

#ifndef EVP_CORE_PACKER_H
#define EVP_CORE_PACKER_H

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <iomanip>

#include "fs.h"
#include "consts.h"
#include "md5.h"

namespace evp {
	namespace core {
		static void file_desc_to_bytes(const file_desc& file_desc, std::vector<unsigned char>& buffer);
		static void update_progress(bool force = false);

		static float curr_prog = 0;
		static int prev_prog = 0;

		constexpr int PROGRESS_UPDATE_PERCENT = 2;
		
		static void pack(std::string input, std::string output) {
			if(evp::fs::is_dir(input.c_str()) == FS_NOT_A_DIR)
				return;

			std::vector<file_desc> input_files;
			size_t curr_data_offset = DATA_START_OFFSET;
			size_t footer_size = 0;

			auto files = evp::fs::get_all_files(input.c_str());

			float prog_change_x = 85.0 / files.size();
			float prog_change_y = 15.0 / files.size();
			update_progress(true);

			std::ofstream fout;
			fout.open(output.c_str(), std::ios::binary);

			fout.write(UNKNOWN_HEADER_BYTES, sizeof(UNKNOWN_HEADER_BYTES));
			fout.write(RESERVED_BYTES, sizeof(RESERVED_BYTES));

			for(std::string file : files) {
				file_desc input_file;

				// save file path
				input_file.m_path = file;

				// get file content
				std::ifstream input_stream(file, std::ios::binary);
				input_file.m_data = std::vector<unsigned char>((std::istreambuf_iterator<char>(input_stream)), (std::istreambuf_iterator<char>()));
				input_stream.close();

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

				curr_prog += prog_change_x;
				update_progress();
			}

			fout.write(RESERVED_BYTES, sizeof(RESERVED_BYTES));

			for(file_desc input_file : input_files) {
				// get path relative to input path
				int start_index = input_file.m_path.find(input.c_str());
				input_file.m_path.erase(start_index, input.size());

				// remove leading slash
				if(input_file.m_path[0] == '/')
					input_file.m_path.erase(0, 1);

				// fix path slashes
				std::replace(input_file.m_path.begin(), input_file.m_path.end(), '/', '\\');

				// save path size
				input_file.m_path_size = input_file.m_path.size();

				// pack file desc
				std::vector<unsigned char> file_desc_bytes;
				file_desc_to_bytes(input_file, file_desc_bytes);

				fout.write((char*)file_desc_bytes.data(), file_desc_bytes.size());

				footer_size += file_desc_bytes.size();

				curr_prog += prog_change_y;
				update_progress();
			}

			uint64_t num_files = input_files.size();

			fout.seekp(HEADER_END_OFFSET, std::ios_base::beg);
			fout.write((char*)&curr_data_offset, sizeof(size_t));
			fout.write((char*)&footer_size, sizeof(size_t));
			fout.write((char*)&num_files, sizeof(uint64_t));

			curr_prog = 100;
			update_progress();

			fout.close();
		}

		static void file_desc_to_bytes(const file_desc& file_desc, std::vector<unsigned char>& buffer) {
			buffer.insert(buffer.end(), (unsigned char*)&(file_desc.m_path_size), (unsigned char*)&(file_desc.m_path_size) + sizeof(size_t));
			buffer.insert(buffer.end(), file_desc.m_path.begin(), file_desc.m_path.end());
			buffer.insert(buffer.end(), (unsigned char*)&(file_desc.m_data_start_offset), (unsigned char*)&(file_desc.m_data_start_offset) + sizeof(size_t));
			buffer.insert(buffer.end(), (unsigned char*)&(file_desc.m_data_size), (unsigned char*)&(file_desc.m_data_size) + sizeof(size_t));
			buffer.insert(buffer.end(), (unsigned char*)&(file_desc.m_data_size), (unsigned char*)&(file_desc.m_data_size) + sizeof(size_t));
			buffer.insert(buffer.end(), 1, 1);
			buffer.insert(buffer.end(), 11, 0);
			buffer.insert(buffer.end(), file_desc.m_data_hash, file_desc.m_data_hash + sizeof(file_desc.m_data_hash));
		}

		static void update_progress(bool force) {
			if(force) {
				std::cout << "\r[";
				for(int i = 1; i <= 100 / PROGRESS_UPDATE_PERCENT; i++) {
					std::cout << " ";
				}
				std::cout << "] Packing    " << std::flush;
				return;
			}

			int progress = curr_prog / PROGRESS_UPDATE_PERCENT;

			if(progress > prev_prog) {
				prev_prog = progress;

				std::cout << "\r[";
				for(int i = 1; i <= 100 / PROGRESS_UPDATE_PERCENT; i++) {
					if(progress >= i)
						std::cout << "X";
					else
						std::cout << " ";
				}

				if(progress != 100 / PROGRESS_UPDATE_PERCENT && progress % 4 == 0)
					std::cout << "] Packing    " << std::flush;
				else if(progress != 100 / PROGRESS_UPDATE_PERCENT && progress % 4 == 1)
					std::cout << "] Packing.   " << std::flush;
				else if(progress != 100 / PROGRESS_UPDATE_PERCENT && progress % 4 == 2)
					std::cout << "] Packing..  " << std::flush;
				else if(progress != 100 / PROGRESS_UPDATE_PERCENT && progress % 4 == 3)
					std::cout << "] Packing... " << std::flush;
				else
					std::cout << "] Done       " << std::flush;
			}
		}
	}
}

#endif
