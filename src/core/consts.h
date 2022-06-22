#pragma once

#ifndef EVP_CORE_CONSTS_H
#define EVP_CORE_CONSTS_H

//#define UNKNOWN_HEADER_BYTES "353235633137613661376366626364373534313265636430363964346237326333383900100000004E4F524D414C5F5041434B5F5459504564000000"

constexpr char UNKNOWN_HEADER_BYTES[60]{ 53,50,53,99,49,55,97,54,97,55,99,102,98,99,100,55,53,52,49,50,101,99,100,48,54,57,100,52,98,55,50,99,51,56,57,0,16,0,0,0,78,79,82,77,65,76,95,80,65,67,75,95,84,89,80,69,100,0,0,0 };
constexpr char RESERVED_BYTES[16]{};

#define DATA_START_OFFSET 76
#define HEADER_END_OFFSET 60
#define OFFSET_BETWEEN_NAMES 36

struct file_desc {
	std::string m_path;
	std::vector<unsigned char> m_data;
	unsigned char m_data_hash[16];
	size_t m_data_size;
	size_t m_path_size;
	size_t m_data_start_offset;
};

#endif
