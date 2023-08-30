#pragma once

#ifndef LIBEVP_H
#define LIBEVP_H

#include <vector>
#include <string>
#include <functional>
#include <filesystem>

#include "libdvsku_crypt/libdvsku_crypt.h"

// Bytes that describe v1 of an evp packer
constexpr char EVP_V1_HEADER[60] {
	53,50,53,99,49,55,97,54,97,55,99,102,98,99,100,55,53,52,49,50,101,99,100,48,54,57,100,52,
	98,55,50,99,51,56,57,0,16,0,0,0,78,79,82,77,65,76,95,80,65,67,75,95,84,89,80,69,100,0,0,0
};

// Padding
constexpr char RESERVED_BYTES[16] {};

// Offset where file data begins
constexpr uint32_t DATA_START_OFFSET = 0x4C;

// Offset where header ends
constexpr uint32_t HEADER_END_OFFSET = 0x3C;

// Offset between two file descriptions
constexpr uint32_t OFFSET_BETWEEN_FILE_DESC = 0x24;

namespace libevp {
	class evp {
	public:
		// void f()
		typedef std::function<void()> notify_start;

		// void f(libdvsku::dv_result)
		typedef std::function<void(libdvsku::dv_result)> notify_finish;

		// void f(float)
		typedef std::function<void(float)> notify_update;

		// void f(libdvsku::dv_result)
		typedef std::function<void(libdvsku::dv_result)> notify_error;

		typedef std::filesystem::path	FILE_PATH;
		typedef std::filesystem::path	FOLDER_PATH;

	public:

		/*
		 *	Packs files in folder into a .evp archive
		 *
		 *	@param input	-> folder path containing files to pack
		 *	@param output	-> file path where to save the created .evp archive
		 *	@param encrypt	-> if true encrypts files with the key provided
		 *	@param key		-> encryption key (ignored if encrypt is false)
		 *	@param filter	->
		 *			none: packs all files in folder;
		 *			client: packs only Talisman Online client related files;
		 *			server: packs only Talisman Online server related files
		 * 
		 *	@returns evp_result	
		 *			m_code == evp_status::ok			packed successfully;
		 *			m_code == evp_status::error			an error occurred during packing, m_msg contains details;
		*/
		static libdvsku::dv_result pack(const FOLDER_PATH& input, const FILE_PATH& output, bool encrypt = false,
			const std::string& key = "", libdvsku::crypt::file_filter filter = libdvsku::crypt::file_filter::none);

		/*
		 *	Unpacks .evp archive contents into a folder
		 *
		 *	@param input	-> file path to .evp archive
		 *	@param output	-> folder path where to save unpacked files
		 *	@param decrypt	-> if true tries to decrypt files with the key provided (auto detects if each file is encrypted)
		 *	@param key		-> encryption key (ignored if encrypt is false)
		 *
		 *	@returns evp_result
		 *			m_code == evp_status::ok			unpacked successfully;
		 *			m_code == evp_status::error			an error occurred during unpacking, m_msg contains details;
		*/
		static libdvsku::dv_result unpack(const FILE_PATH& input, const FOLDER_PATH& output,
			bool decrypt = false, const std::string& key = "");

		/*
		 *	Asynchronously packs files in folder into a .evp archive
		 *
		 *	@param input	-> folder path containing files to pack
		 *	@param output	-> file path where to save the created .evp archive
		 *	@param encrypt	-> if true encrypts files with the key provided
		 *	@param key		-> encryption key (ignored if encrypt is false)
		 *	@param filter	->
		 *			none: packs all files in folder;
		 *			client: packs only Talisman Online client related files;
		 *			server: packs only Talisman Online server related files
		 *  
		 *	@param cancel	-> pointer to bool that cancels packing if value is true
		 *  @param started	-> callback that's called when packing starts
		 *  @param update	-> callback that's called when there's progress update
		 *  @param finished -> callback that's called when packing ends
		 *  @param error	-> callback that's called when an error occurres
		 *
		 *	@returns evp_result
		 *			m_code == evp_status::ok			packed successfully;
		 *			m_code == evp_status::error			an error occurred during packing, m_msg contains details;
		 *			m_code == evp_status::cancelled		packing cancelled by user
		*/
		static void pack_async(const FOLDER_PATH& input, const FILE_PATH& output, bool encrypt = false, 
			const std::string& key = "", libdvsku::crypt::file_filter filter = libdvsku::crypt::file_filter::none,
			const bool* cancel = nullptr, notify_start started = nullptr, notify_update update = nullptr,
			notify_finish finished = nullptr, notify_error error = nullptr);

		/*
		 *	Asynchronously unpacks .evp archive contents into a folder
		 *
		 *	@param input	-> file path to .evp archive
		 *	@param output	-> folder path where to save unpacked files
		 *	@param decrypt	-> if true tries to decrypt files with the key provided (auto detects if each file is encrypted)
		 *	@param key		-> encryption key (ignored if encrypt is false)
		 *	@param cancel	-> pointer to bool that cancels packing if value is true
		 *  @param started	-> callback that's called when packing starts
		 *  @param update	-> callback that's called when there's progress update
		 *  @param finished -> callback that's called when packing ends
		 *  @param error	-> callback that's called when an error occurres
		 * 
		 *	@returns evp_result
		 *			m_code == evp_status::ok			unpacked successfully;
		 *			m_code == evp_status::error			an error occurred during unpacking, m_msg contains details;
		 *			m_code == evp_status::cancelled		unpacking cancelled by user
		*/
		static void unpack_async(const FILE_PATH& input, const FOLDER_PATH& output,
			bool decrypt = false, const std::string& key = "", const bool* cancel = nullptr, 
			notify_start started = nullptr, notify_update update = nullptr,
			notify_finish finished = nullptr, notify_error error = nullptr);

		/*
		 *	Checks if the file is encrypted
		 * 
		 *	@param input -> file path
		*/
		static bool is_encrypted(const FILE_PATH& input);
	};
}

#endif
