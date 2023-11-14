#pragma once

#include <vector>
#include <filesystem>

namespace libevp {
    enum class file_filter;

    class filtering {
    public:
        // Get paths to files to pack with filtering
        static std::vector<std::filesystem::path> get_filtered_paths(const std::filesystem::path& dir, file_filter filter);
    };
}