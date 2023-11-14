#include "utilities/filtering.hpp"

using namespace libevp;

// Folders to include when filtering for client_only
const std::vector<std::string> CLIENT_FOLDERS {
    "local", "maps", "model", "model2", "script", "ui", "audio", "music", "scene"
};

// Folders to include when filtering for server_only
const std::vector<std::string> SERVER_FOLDERS {
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

// Extensions to ignore when encrypting
const std::vector<std::string> IGNORE_ENCRYPT_EXTENSIONS {
    ".wav", ".ogg", ".db", ".ifl"
};

bool path_contains_dir(const std::filesystem::path& path, const std::vector<std::string>& dirs);
bool path_contains_filename(const std::filesystem::path& path, const std::vector<std::string>& files);
bool path_contains_extension(const std::filesystem::path& path, const std::vector<std::string>& extensions);

std::vector<std::filesystem::path> filtering::get_filtered_paths(const std::filesystem::path& dir, file_filter filter) {
    std::vector<std::filesystem::path> results;

    for (auto const& dir_entry : std::filesystem::recursive_directory_iterator(dir)) {
        if (!std::filesystem::is_regular_file(dir_entry.path())) continue;

        switch (filter) {
            case file_filter::client_only: {
                if (path_contains_dir(dir_entry.path(), CLIENT_FOLDERS) || path_contains_filename(dir_entry.path(), CLIENT_FILES))
                    results.push_back(dir_entry.path());

                break;
            }
            case file_filter::server_only: {
                if (path_contains_dir(dir_entry.path(), SERVER_FOLDERS) || path_contains_filename(dir_entry.path(), SERVER_FILES))
                    results.push_back(dir_entry.path());

                break;
            }
            default: results.push_back(dir_entry.path()); break;
        }
    }

    return results;
}

bool filtering::should_be_encrypted(const std::filesystem::path& file) {
    return !path_contains_extension(file, IGNORE_ENCRYPT_EXTENSIONS);
}

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