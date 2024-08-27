#pragma once

#include "libevp/evp_defs.hpp"
#include "libevp/model/evp_filter.hpp"

#include <vector>

namespace libevp::filtering {
    /*
        Get filtered input files.
    */
    std::vector<FILE_PATH> get_filtered_paths(const DIR_PATH& input, evp_filter filter);
}
