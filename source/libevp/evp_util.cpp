#include "libevp/evp_util.hpp"
#include "libevp/utilities/filtering.hpp"

using namespace libevp;

std::vector<FILE_PATH> util::filter_files(const DIR_PATH& input, evp_filter filter) {
    return filtering::get_filtered_paths(input, filter);
}
