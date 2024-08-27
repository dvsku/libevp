#pragma once

#include <libevp/evp_defs.hpp>
#include <libevp/model/evp_filter.hpp>

namespace libevp::util {
    /*
     *  Get filtered files from an input directory.
     *
     *  @param input    -> input dir
     *  @param filter   -> filter
     *
     *  @returns std::vector<file_path_t> -> filtered files
    */
    LIBEVP_API std::vector<FILE_PATH> filter_files(const DIR_PATH& input, evp_filter filter);
}
