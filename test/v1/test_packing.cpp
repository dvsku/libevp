#include "libevp.hpp"
#include "test/utilities_assert.hpp"

#include <fstream>
#include <iterator>
#include <string>
#include <algorithm>

using namespace libevp;

bool compare_files(const std::string& p1, const std::string& p2) {
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

int v1_packing_single_file(const std::string& base) {
    std::string input  = base + std::string("/test/v1/resources/files_to_pack/subfolder_2");
    std::string output = base + std::string("/test/v1/resources/v1_packing_single_file.evp");
    std::string valid  = base + std::string("/test/v1/resources/valid_single_file.evp");

    try {
        auto r1 = evp::pack(input, output);

        if (!r1)
            std::remove(output.c_str());

        ASSERT(r1);

        auto r2 = compare_files(output, valid);
        std::remove(output.c_str());

        ASSERT(r2 == true);
    }
    catch (...) { return 1; }

    return 0;
}

int v1_packing_folder(const std::string& base) {
    std::string input  = base + std::string("/test/v1/resources/files_to_pack");
    std::string output = base + std::string("/test/v1/resources/v1_packing_folder.evp");
    std::string valid  = base + std::string("/test/v1/resources/valid_folders.evp");

    try {
        auto r1 = evp::pack(input, output);

        if (!r1)
            std::remove(output.c_str());

        ASSERT(r1);

        auto r2 = compare_files(output, valid);
        std::remove(output.c_str());

        ASSERT(r2 == true);
    }
    catch (...) { return 1; }

    return 0;
}

int main(int argc, char* argv[]) {
    if (argc == 3) {
        switch (std::stoi(argv[1])) {
            case 0: return v1_packing_single_file(argv[2]);
            case 1: return v1_packing_folder(argv[2]);
            default: break;
        }
    }

    return 0;
}