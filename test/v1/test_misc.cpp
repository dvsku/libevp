#include "libevp.hpp"
#include "test/utilities_assert.hpp"

using namespace libevp;

int v1_get_evp_file_list(const std::string& base) {
	std::string input = base + std::string("/test/v1/resources/valid_folders.evp");

	auto files = evp::get_evp_file_list(input);

	ASSERT(files.size() == 4);
	ASSERT(files[0].generic_string() == "subfolder_1/text_1.txt");
	ASSERT(files[1].generic_string() == "subfolder_1/text_2.txt");
	ASSERT(files[2].generic_string() == "subfolder_2/text_3.txt");
	ASSERT(files[3].generic_string() == "text_1.txt");

    return 0;
}

int main(int argc, char* argv[]) {
	if (argc == 3) {
		switch (std::stoi(argv[1])) {
			case 0: return v1_get_evp_file_list(argv[2]);
			default: break;
		}
	}

	return 0;
}