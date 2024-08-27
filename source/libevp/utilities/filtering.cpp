#include "filtering.hpp"
#include "libevp.hpp"

using namespace libevp;

////////////////////////////////////////////////////////////////////////////////
// INTERNAL

// Directories to include when filtering for client_only
const std::vector<std::string> CLIENT_DIRS {
    "local", "maps", "model", "model2", "script", "ui", "audio", "music", "scene"
};

// Directories to include when filtering for server_only
const std::vector<std::string> SERVER_DIRS {
    "local", "maps", "script"
};

// Files to include when filtering for client_only
const std::vector<std::string> CLIENT_FILES {
    "client_engine.ini", "client_game.ini"
};

// Files to include when filtering for server_only
const std::vector<std::string> SERVER_FILES {
    "server_engine.ini", "server_game.ini", "server_user.ini"
};

static bool path_contains_dir(const std::filesystem::path& path, const std::vector<std::string>& dirs);
static bool path_contains_filename(const std::filesystem::path& path, const std::vector<std::string>& files);
static bool path_contains_extension(const std::filesystem::path& path, const std::vector<std::string>& extensions);

////////////////////////////////////////////////////////////////////////////////
// PUBLIC

std::vector<FILE_PATH> filtering::get_filtered_paths(const DIR_PATH& input, evp_filter filter) {
    std::vector<std::filesystem::path> results;

    for (auto const& dir_entry : std::filesystem::recursive_directory_iterator(input)) {
        if (!std::filesystem::is_regular_file(dir_entry.path())) continue;

        switch (filter) {
            case evp_filter::client_only: {
                if (path_contains_dir(dir_entry.path(), CLIENT_DIRS) || path_contains_filename(dir_entry.path(), CLIENT_FILES))
                    results.push_back(dir_entry.path());

                break;
            }
            case evp_filter::server_only: {
                if (path_contains_dir(dir_entry.path(), SERVER_DIRS) || path_contains_filename(dir_entry.path(), SERVER_FILES))
                    results.push_back(dir_entry.path());

                break;
            }
            default: results.push_back(dir_entry.path()); break;
        }
    }

    return results;
}

////////////////////////////////////////////////////////////////////////////////
// INTERNAL

bool path_contains_dir(const std::filesystem::path& path, const std::vector<std::string>& dirs) {
    const std::string dir = path.parent_path().generic_string();
    for (auto& current_dir : dirs) {
        if (dir.find(current_dir) != std::string::npos)
            return true;
    }
    return false;
}

bool path_contains_filename(const std::filesystem::path& path, const std::vector<std::string>& filenames) {
    for (auto& filename : filenames) {
        if (path.filename().generic_string() == filename)
            return true;
    }
    return false;
}

bool path_contains_extension(const std::filesystem::path& path, const std::vector<std::string>& extensions) {
    for (auto& extension : extensions) {
        if (path.extension().generic_string() == extension)
            return true;
    }
    return false;
}
