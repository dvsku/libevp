#pragma once

#ifndef EVP_CORE_PACKER_H
#define EVP_CORE_PACKER_H

#include <string>

#include "boost/filesystem/path.hpp"
#include "boost/filesystem/operations.hpp"

namespace fs = boost::filesystem;

namespace evp {
	namespace core {
		static void pack(std::string input, std::string output) {
			fs::path input_path(input);
			fs::path output_path(output);

			if(!fs::exists(input_path))
				throw std::runtime_error("invalid input folder");

			
		}
	}
}

#endif
