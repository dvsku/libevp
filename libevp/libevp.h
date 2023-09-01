#pragma once

#include <vector>
#include <string>
#include <functional>
#include <filesystem>

#include "lib/libdvsku_crypt/libdvsku_crypt.h"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	#if !defined(NDEBUG) 
		#pragma comment(lib, "lib/libdvsku_crypt/libdvsku_crypt_debug")
	#else 
		#pragma comment(lib, "lib/libdvsku_crypt/libdvsku_crypt_release")
	#endif
#elif defined(__linux__) || defined(__unix__)
	#if !defined(NDEBUG) 
		#error Missing libdvsku_crypt lib
	#else 
		#error Missing libdvsku_crypt lib
	#endif
#else
	#error Unsupported platform
#endif

namespace libevp {
		client_only,	// include only Talisman Online client files
		server_only		// include only Talisman Online server files
	};

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

		static std::vector<FILE_PATH> get_file_list(const FILE_PATH& input, bool decrypt = false, const std::string& key = "");

		/*
		 *	Checks if the file is encrypted
		 * 
		 *	@param input -> file path
		*/
		static bool is_encrypted(const FILE_PATH& input);
	};
}
