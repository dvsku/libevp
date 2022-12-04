#pragma once

#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <vector>

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>

namespace filesys = std::experimental::filesystem;

constexpr char UNKNOWN_HEADER_BYTES[60] { 
	53,50,53,99,49,55,97,54,97,55,99,102,98,99,100,55,53,52,49,50,101,99,100,48,54,57,100,52,
	98,55,50,99,51,56,57,0,16,0,0,0,78,79,82,77,65,76,95,80,65,67,75,95,84,89,80,69,100,0,0,0 
};

constexpr char RESERVED_BYTES[16] {};

constexpr uint32_t DATA_START_OFFSET = 76;
constexpr uint32_t HEADER_END_OFFSET = 60;
constexpr uint32_t OFFSET_BETWEEN_NAMES = 36;

#define BUF8 std::vector<uint8_t>

enum evp_status : unsigned int {
	ok,
	error,
};

#endif
