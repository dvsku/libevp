#include <libevp.hpp>
#include <gtest/gtest.h>

#include <fstream>
#include <iterator>
#include <string>
#include <algorithm>
#include <regex>

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

static bool compare_buffers(const std::vector<uint8_t>& b1, const std::vector<uint8_t>& b2) {
    if (b1.size() != b2.size())
        return false;

    return std::equal(b1.begin(), b1.end(), b2.begin());
}

TEST(unpacking, v1_unpacking) {
    evp evp;

    evp::unpack_input input;
    input.archive = BASE_PATH + std::string("/tests/v1/resources/single_file.evp");

    std::string output      = BASE_PATH + std::string("/tests/v1/resources/unpack_here/");
    std::string valid       = BASE_PATH + std::string("/tests/v1/resources/files_to_pack/subfolder_2/text_3.txt");
    std::string output_file = BASE_PATH + std::string("/tests/v1/resources/unpack_here/text_3.txt");

    std::filesystem::create_directories(output);

    auto r1 = evp.unpack(input, output);

    EXPECT_TRUE(r1);
    EXPECT_TRUE(compare_files(output_file, valid));

    std::filesystem::remove_all(output);
}

TEST(unpacking, v1_get_file) {
    evp evp;

    std::string input = BASE_PATH + std::string("/tests/v1/resources/multiple_files.evp");
    std::string valid = BASE_PATH + std::string("/tests/v1/resources/files_to_pack/text_1.txt");

    std::vector<uint8_t> buffer;

    auto r1 = evp.get_file(input, "text_1.txt", buffer);
    ASSERT_TRUE(r1);

    std::ifstream stream(valid, std::ios::in | std::ios::binary);
    ASSERT_TRUE(stream.is_open());

    std::vector<uint8_t> contents((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    ASSERT_TRUE(compare_buffers(buffer, contents));
}

TEST(unpacking, v1_get_file_stream) {
    evp evp;

    std::string input = BASE_PATH + std::string("/tests/v1/resources/multiple_files.evp");
    std::string valid = BASE_PATH + std::string("/tests/v1/resources/files_to_pack/text_1.txt");

    std::stringstream ss;

    auto r1 = evp.get_file(input, "text_1.txt", ss);
    ASSERT_TRUE(r1);

    std::ifstream stream(valid, std::ios::in | std::ios::binary);
    ASSERT_TRUE(stream.is_open());

    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(ss)), std::istreambuf_iterator<char>());
    std::vector<uint8_t> contents((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

    ASSERT_TRUE(compare_buffers(buffer, contents));
}
