#pragma once

#include <vector>
#include <filesystem>

#include "lib/libdvsku_crypt/libdvsku_crypt.h"

namespace libevp {
	class filtering {
	public:
		// Get paths to files to pack with filtering
		static std::vector<std::filesystem::path> get_filtered_paths(const std::filesystem::path& dir, 
			libdvsku::crypt::file_filter filter = libdvsku::crypt::file_filter::none);

		static bool should_be_encrypted(const std::filesystem::path& file);
	};
}