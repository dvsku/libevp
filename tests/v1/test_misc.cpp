#include <libevp.hpp>
#include <gtest/gtest.h>

using namespace libevp;

TEST(misc, get_evp_file_list) {
    evp         evp;
    std::string input = BASE_PATH + std::string("/tests/v1/resources/valid_folders.evp");

    auto files = evp.get_evp_file_list(input);

    EXPECT_TRUE(files.size() == 4);
    EXPECT_TRUE(files[0].generic_string() == "subfolder_1/text_1.txt");
    EXPECT_TRUE(files[1].generic_string() == "subfolder_1/text_2.txt");
    EXPECT_TRUE(files[2].generic_string() == "subfolder_2/text_3.txt");
    EXPECT_TRUE(files[3].generic_string() == "text_1.txt");
}
