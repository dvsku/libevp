#pragma once

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    #if defined LIBEVP_DLL
        #define LIBEVP_API __declspec(dllexport)
    #else
        #define LIBEVP_API
    #endif
#else
    #define LIBEVP_API
#endif

#include <filesystem>

namespace libevp {
    using FILE_PATH = std::filesystem::path;
    using DIR_PATH  = std::filesystem::path;
}
