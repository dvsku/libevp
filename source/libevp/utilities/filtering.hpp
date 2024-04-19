#pragma once

#include <vector>
#include <filesystem>

namespace libevp {
    enum class evp_filter : uint32_t;

    class filtering {
    public:
        // Get paths to files to pack with filtering
        static std::vector<std::filesystem::path> get_filtered_paths(const std::filesystem::path& dir, evp_filter filter);
    };
}