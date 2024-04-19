#include <libevp.hpp>
#include <gtest/gtest.h>

#include <fstream>
#include <iterator>
#include <string>
#include <algorithm>

using namespace libevp;

static bool compare_files(const std::string& p1, const std::string& p2) {
    std::ifstream f1(p1, std::ifstream::binary | std::ifstream::ate);
    std::ifstream f2(p2, std::ifstream::binary | std::ifstream::ate);

    if (f1.fail() || f2.fail()) {
        return false;
    }

    if (f1.tellg() != f2.tellg()) {
        return false;
    }

    f1.seekg(0, std::ifstream::beg);
    f2.seekg(0, std::ifstream::beg);
    return std::equal(std::istreambuf_iterator<char>(f1.rdbuf()),
        std::istreambuf_iterator<char>(),
        std::istreambuf_iterator<char>(f2.rdbuf()));
}

TEST(packing, v1_packing_single_file) {
    std::string input  = BASE_PATH + std::string("/tests/v1/resources/files_to_pack/subfolder_2");
    std::string output = BASE_PATH + std::string("/tests/v1/resources/v1_packing_single_file.evp");
    std::string valid  = BASE_PATH + std::string("/tests/v1/resources/valid_single_file.evp");

    auto r1 = evp::pack(input, output);

    EXPECT_TRUE(r1.status == evp_result::e_status::ok);
    EXPECT_TRUE(compare_files(output, valid));

    std::remove(output.c_str());
}

TEST(packing, v1_packing_folder) {
    std::string input  = BASE_PATH + std::string("/tests/v1/resources/files_to_pack");
    std::string output = BASE_PATH + std::string("/tests/v1/resources/v1_packing_folder.evp");
    std::string valid  = BASE_PATH + std::string("/tests/v1/resources/valid_folders.evp");

    auto r1 = evp::pack(input, output);

    EXPECT_TRUE(r1.status == evp_result::e_status::ok);
    EXPECT_TRUE(compare_files(output, valid));
    
    std::remove(output.c_str());
}
