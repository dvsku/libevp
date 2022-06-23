#include <iostream>

#include "core/packer.h"
#include "core/unpacker.hpp"

void print_usage();

using namespace std;

int main(int argc, char* argv[]) {
	if(argc != 4) {
		print_usage();
		return 1;
	}

	filesys::path input(argv[2]);
	filesys::path output(argv[3]);

	if (!input.is_absolute())
		input = filesys::absolute(input);

	if (!output.is_absolute())
		output = filesys::absolute(output);

	if(strcmp(argv[1], "pack") != 0) {
		try {
			if(!filesys::exists(input))
				throw runtime_error("Input directory doesn't exist");

			if (!filesys::is_directory(input))
				throw runtime_error("Input has to be a directory");

			if (filesys::is_directory(output))
				throw runtime_error("Output cannot be a directory");

			if (!output.has_filename())
				throw runtime_error("Output must be a file with .evp extension");

			if (!output.has_extension() || output.extension() != ".evp")
				throw runtime_error("Output extension must be .evp");

			evp::core::pack(input, output);
		}
		catch(const exception& ex) {
			cout << "Runtime error: " << ex.what() << endl;
		}
	}
	else if(strcmp(argv[1], "unpack") == 0) {
		try {
			if (!filesys::exists(output))
				throw runtime_error("Output directory doesn't exist");

			if (!filesys::is_directory(output))
				throw runtime_error("Output has to be a directory");

			if (filesys::is_directory(input))
				throw runtime_error("Input cannot be a directory");

			if (!input.has_filename())
				throw runtime_error("Input must be a file with .evp extension");

			if (!input.has_extension() || input.extension() != ".evp")
				throw runtime_error("Input extension must be .evp");

			evp::core::unpack(input, output);
		}
		catch (const exception& ex) {
			cout << "Runtime error: " << ex.what() << endl;
		}
	}
	else {
		print_usage();
		return 2;
	}

	return 0;
}

void print_usage() {
	cout << "Usage:" << endl;
	cout << "\t" << "pack input_folder output_file" << endl;
	cout << "\t" << "unpack input_file output_folder" << endl;
}