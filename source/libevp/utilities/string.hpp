#pragma once

#define FMT_HEADER_ONLY
#include <fmt/format.h>

#define EVP_STR_FORMAT(frmt, ...) fmt::format(fmt::runtime(frmt), ##__VA_ARGS__)