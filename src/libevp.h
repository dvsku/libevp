#pragma once

#ifndef LIBEVP_H
#define LIBEVP_H

#include <vector>
#include <string>
#include <functional>

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>

namespace filesys = std::experimental::filesystem;

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

namespace dvsku {
	class evp {
		protected:
			struct file_desc {
				std::string m_path;
				std::vector<uint8_t> m_data;
				unsigned char m_data_hash[16];
				size_t m_data_size;
				size_t m_path_size;
				size_t m_data_start_offset;
			};

		public:
			enum file_filter : unsigned int {
				none,			// include all files
				client_only,	// include client only files
				server_only		// include server only files
			};

			enum evp_status : unsigned int {
				ok,
				error,
				cancelled
			};

			struct evp_result {
				evp_status m_code = evp_status::ok;
				std::string m_msg = "";

				evp_result() {}
				evp_result(evp_status code) : m_code(code) {}
				evp_result(evp_status code, const std::string& msg) : m_code(code), m_msg(msg) {}
			};

			// void f()
			typedef std::function<void()> notify_start;

			// void f(dvsku::evp::evp_status)
			typedef std::function<void(dvsku::evp::evp_status)> notify_finish;

			// void f(float)
			typedef std::function<void(float)> notify_update;

			// void f(dvsku::evp::evp_status)
			typedef std::function<void(dvsku::evp::evp_result)> notify_error;

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
			static evp_result pack(const std::string& input, const std::string& output,
				bool encrypt = false, const std::string& key = "", const file_filter& filter = file_filter::none);

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
			static evp_result unpack(const std::string& input, const std::string& output,
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
			static void pack_async(const std::string& input, const std::string& output,
				bool encrypt = false, const std::string& key = "", const file_filter& filter = file_filter::none,
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
			static void unpack_async(const std::string& input, const std::string& output,
				bool decrypt = false, const std::string& key = "", const bool* cancel = nullptr, 
				notify_start started = nullptr, notify_update update = nullptr,
				notify_finish finished = nullptr, notify_error error = nullptr);

			/*
			 *	Checks if the file is encrypted
			 * 
			 *	@param input -> file path
			*/
			static bool is_encrypted(const std::string& input);

		protected:
			static evp_result pack_impl(const filesys::path& input, const filesys::path& output,
				bool encrypt = false, const std::string& key = "", const file_filter& filter = file_filter::none,
				const bool* cancel = nullptr, notify_start started = nullptr, notify_update update = nullptr,
				notify_finish finished = nullptr, notify_error error = nullptr);

			static evp_result unpack_impl(const filesys::path& input, const filesys::path& output,
				bool decrypt = false, const std::string& key = "", const bool* cancel = nullptr, 
				notify_start started = nullptr, notify_update update = nullptr,
				notify_finish finished = nullptr, notify_error error = nullptr);

			static void serialize_file_desc(const file_desc& file_desc, std::vector<unsigned char>& buffer);
			
			static evp_result is_evp_header_valid(const filesys::path& input);

			static evp_result are_pack_paths_valid(const std::string& input, const std::string& output);

			static evp_result are_unpack_paths_valid(const std::string& input, const std::string& output);

			static std::vector<filesys::path> get_files(const filesys::path& dir, const file_filter& filter = file_filter::none);
	};
}

#endif
