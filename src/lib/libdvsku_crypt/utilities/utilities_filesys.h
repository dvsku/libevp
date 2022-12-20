#pragma once

#ifndef UTILITIES_FILESYS_H
#define UTILITIES_FILESYS_H

#include <vector>

#ifndef _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#endif

#include <experimental/filesystem>

namespace std_filesys = std::experimental::filesystem;

#define FILE_PATH std_filesys::path
#define FILE_PATH_REF_C const std_filesys::path&

#define FOLDER_PATH std_filesys::path
#define FOLDER_PATH_REF_C const std_filesys::path&

#define FILE_OR_FOLDER_PATH std_filesys::path
#define FILE_OR_FOLDER_PATH_REF_C const std_filesys::path&

#define FILE_FILTER_NONE dvsku::filesys::utilities::file_filter::none
#define FILE_FILTER_CLIENT dvsku::filesys::utilities::file_filter::client_only
#define FILE_FILTER_SERVER dvsku::filesys::utilities::file_filter::server_only
#define FILE_FILTER_REF_C const dvsku::filesys::utilities::file_filter&

namespace dvsku::filesys {
	class utilities {
		protected:
			// Folders to include when packing evp
			static inline const std::vector<std::string> CLIENT_FOLDERS_EVP {
				"local", "maps", "model", "model2", "script", "ui", "audio", "music", "scene"
			};

			// Folders to include when encrypting/compressing
			static inline const std::vector<std::string> CLIENT_FOLDERS_CRYPT {
				"local", "maps", "model", "model2", "script", "ui", "scene"
			};

			// Files to include
			static inline const std::vector<std::string> CLIENT_FILES {
				"client_engine.ini", "client_game.ini"
			};

			// Extensions to exclude
			static inline const std::vector<std::string> EXCLUDE_CLIENT_EXTENSIONS {
				".wav", ".ogg", ".db", ".ifl"
			};

			// Folders to include
			static inline const std::vector<std::string> SERVER_FOLDERS {
				"local", "maps", "script"
			};

			// Files to include
			static inline const std::vector<std::string> SERVER_FILES {
				"server_engine.ini", "server_game.ini", "server_user.ini"
			};

			// Extensions to exclude
			static inline const std::vector<std::string> EXCLUDE_SERVER_EXTENSIONS {
				""
			};

			static bool compare_folders(const std_filesys::path& path, const std::vector<std::string>& folders) {
				const std::string dir = path.parent_path().u8string();
				for (const std::string& folder : folders) {
					if (dir.find(folder) != std::string::npos)
						return true;
				}
				return false;
			}

			static bool compare_files(const std_filesys::path& path, const std::vector<std::string>& files) {
				for (const std::string& file : files) {
					if (path.filename().u8string() == file)
						return true;
				}
				return false;
			}

			static bool compare_extensions(const std_filesys::path& path, const std::vector<std::string>& extensions) {
				for (const std::string& extension : extensions) {
					if (path.extension().u8string() == extension)
						return true;
				}
				return false;
			}

		public:
			enum file_filter : unsigned int {
				none,			// include all files
				client_only,	// include only Talisman Online client files
				server_only		// include only Talisman Online server files
			};

			static std::vector<std_filesys::path> get_files_crypt(const std_filesys::path& dir, 
				FILE_FILTER_REF_C filter = FILE_FILTER_NONE) 
			{
				std::vector<std_filesys::path> results;

				for (auto const& dir_entry : std_filesys::recursive_directory_iterator(dir)) {
					if (std_filesys::is_regular_file(dir_entry.path())) {
						switch (filter) {
						case client_only: {
							if (!compare_folders(dir_entry.path(), CLIENT_FOLDERS_CRYPT)) {
								if (compare_files(dir_entry.path(), CLIENT_FILES))
									results.push_back(dir_entry.path());
							}
							else {
								if (!compare_extensions(dir_entry.path(), EXCLUDE_CLIENT_EXTENSIONS))
									results.push_back(dir_entry.path());
							}

							break;
						}
						case server_only: {
							if (!compare_folders(dir_entry.path(), SERVER_FOLDERS)) {
								if (compare_files(dir_entry.path(), SERVER_FILES))
									results.push_back(dir_entry.path());
							}
							else {
								if (!compare_extensions(dir_entry.path(), EXCLUDE_SERVER_EXTENSIONS))
									results.push_back(dir_entry.path());
							}

							break;
						}
						default: results.push_back(dir_entry.path()); break;
						}
					}
				}

				return results;
			}

			static std::vector<std_filesys::path> get_files_evp(const std_filesys::path& dir, 
				FILE_FILTER_REF_C filter = FILE_FILTER_NONE) 
			{
				std::vector<std_filesys::path> results;

				for (auto const& dir_entry : std_filesys::recursive_directory_iterator(dir)) {
					if (std_filesys::is_regular_file(dir_entry.path())) {
						switch (filter) {
							case client_only: {
								if (!compare_folders(dir_entry.path(), CLIENT_FOLDERS_EVP)) {
									if (compare_files(dir_entry.path(), CLIENT_FILES))
										results.push_back(dir_entry.path());
								}
								else {
									results.push_back(dir_entry.path());
								}

								break;
							}
							case server_only: {
								if (!compare_folders(dir_entry.path(), SERVER_FOLDERS)) {
									if (compare_files(dir_entry.path(), SERVER_FILES))
										results.push_back(dir_entry.path());
								}
								else {
									results.push_back(dir_entry.path());
								}

								break;
							}
							default: results.push_back(dir_entry.path()); break;
						}
					}
				}

				return results;
			}
	};
}

#endif