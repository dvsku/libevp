#include <libevp.hpp>
#include <gtest/gtest.h>

using namespace libevp;

TEST(misc, get_files) {
    evp         evp;
    std::string input = BASE_PATH + std::string("/tests/v1/resources/multiple_files.evp");

    std::vector<evp_fd> files = {};
    auto result = evp.get_files(input, files);

    ASSERT_TRUE(result);
    ASSERT_TRUE(files.size() == 4);
    EXPECT_TRUE(files[0].file == "subfolder_1/text_1.txt");
    EXPECT_TRUE(files[1].file == "subfolder_1/text_2.txt");
    EXPECT_TRUE(files[2].file == "subfolder_2/text_3.txt");
    EXPECT_TRUE(files[3].file == "text_1.txt");
}

TEST(misc, validate_files) {
    evp         evp;
    std::string input = BASE_PATH + std::string("/tests/v1/resources/multiple_files.evp");

    auto result = evp.validate_files(input);
    ASSERT_TRUE(result);
}
