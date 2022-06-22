#pragma once

#ifndef EVP_FS_H
#define EVP_FS_H

#include <vector>

#include "tinydir.h"

#define FS_IS_A_DIR 0
#define FS_NOT_A_DIR -1

namespace evp {
	namespace fs {
		static std::vector<std::string> get_all_files(const char* path) {
			std::vector<std::string> files;
			tinydir_dir dir;

			if(tinydir_open(&dir, path) == -1) {
				tinydir_close(&dir);
				return files;
			}

			while(dir.has_next) {
				tinydir_file file;

				if(tinydir_readfile(&dir, &file) == -1) {
					break;
				}

				if(file.is_dir && file.name[0] == '.') {
					if(tinydir_next(&dir) == -1) {
						break;
					}
					continue;
				}

				if(file.is_dir) {
					auto t_files = get_all_files(file.path);
					files.insert(files.end(), t_files.begin(), t_files.end());
				}
				else {
					files.push_back(std::string(file.path));
				}

				if(tinydir_next(&dir) == -1) {
					break;
				}
			}

			tinydir_close(&dir);
			return files;
		}

		static int is_dir(const char* path) {
			tinydir_dir dir;

			if(tinydir_open(&dir, path) == -1) {
				tinydir_close(&dir);
				return FS_NOT_A_DIR;
			}

			return FS_IS_A_DIR;
		}
	}
}

#endif