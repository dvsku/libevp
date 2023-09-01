#pragma once

#include <vector>
#include <filesystem>

namespace libevp {
	enum class file_filter : unsigned char {
		none,			// include all files
		client_only,	// include only Talisman Online client files
		server_only		// include only Talisman Online server files
	};

	class filtering {
	public:
		// Get paths to files to pack with filtering
		static std::vector<std::filesystem::path> get_filtered_paths(const std::filesystem::path& dir, 
			file_filter filter = file_filter::none);

		static bool should_be_encrypted(const std::filesystem::path& file);
	};
}