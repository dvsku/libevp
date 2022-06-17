#include <iostream>

//#pragma comment(lib, "libs/libboost_filesystem")

#include "core/packer.h"

void print_usage();

using namespace std;

int main(int argc, char* argv[]) {

	if(argc != 4) {
		print_usage();
		return 1;
	}

	if(strcmp(argv[1], "pack") == 0) {
		try {
			evp::core::pack(string(argv[2]), string(argv[3]));
		}
		catch(const exception& ex) {
			cout << "Runtime error:" << endl;
			cout << "\t" << ex.what() << endl;
		}
	}
	else if(strcmp(argv[1], "unpack") == 0) {
		
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