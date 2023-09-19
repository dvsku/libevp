#pragma once

#include <vector>
#include <string>
#include <functional>
#include <filesystem>

#include "utilities/filtering.hpp"
#include "utilities/dv_result.hpp"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	#if defined LIBEVP_DLL
		#define LIBEVP_API __declspec(dllexport)
	#else
		#define LIBEVP_API
	#endif
#else
	#define LIBEVP_API 
#endif

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
		typedef std::vector<uint8_t>	BUFFER;

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
		 *			status == dv_status::ok				packed successfully;
		 *			status == dv_status::error			an error occurred during packing, msg contains details;
		*/
		static LIBEVP_API libdvsku::dv_result pack(const FOLDER_PATH& input_dir, const FILE_PATH& evp, bool encrypt = false,
			const std::string& key = "", file_filter filter = file_filter::none);

		/*
		 *	Unpacks .evp archive contents into a folder
		 *
		 *	@param input	-> file path to .evp archive
		 *	@param output	-> folder path where to save unpacked files
		 *	@param decrypt	-> if true tries to decrypt files with the key provided (auto detects if each file is encrypted)
		 *	@param key		-> decryption key (ignored if encrypt is false)
		 *
		 *	@returns evp_result
		 *			status == dv_status::ok				unpacked successfully;
		 *			status == dv_status::error			an error occurred during unpacking, msg contains details;
		*/
		static LIBEVP_API libdvsku::dv_result unpack(const FILE_PATH& evp, const FOLDER_PATH& output_dir,
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
		 *			status == dv_status::ok				packed successfully;
		 *			status == dv_status::error			an error occurred during packing, msg contains details;
		 *			status == dv_status::cancelled		packing cancelled by user
		*/
		static LIBEVP_API void pack_async(const FOLDER_PATH& input_dir, const FILE_PATH& evp, bool encrypt = false,
			const std::string& key = "", file_filter filter = file_filter::none,
			const bool* cancel = nullptr, notify_start started = nullptr, notify_update update = nullptr,
			notify_finish finished = nullptr, notify_error error = nullptr);

		/*
		 *	Asynchronously unpacks .evp archive contents into a folder
		 *
		 *	@param evp			-> file path to .evp archive
		 *	@param output_dir	-> folder path where to save unpacked files
		 *	@param decrypt		-> if true tries to decrypt files with the key provided (auto detects if each file is encrypted)
		 *	@param key			-> decryption key (ignored if encrypt is false)
		 *	@param cancel		-> pointer to bool that cancels unpacking if value is true
		 *  @param started		-> callback that's called when unpacking starts
		 *  @param update		-> callback that's called when there's progress update
		 *  @param finished		-> callback that's called when unpacking ends
		 *  @param error		-> callback that's called when an error occurres
		 * 
		 *	@returns evp_result
		 *			status == dv_status::ok				unpacked successfully;
		 *			status == dv_status::error			an error occurred during unpacking, msg contains details;
		 *			status == dv_status::cancelled		unpacking cancelled by user
		*/
		static LIBEVP_API void unpack_async(const FILE_PATH& evp, const FOLDER_PATH& output_dir,
			bool decrypt = false, const std::string& key = "", const bool* cancel = nullptr, 
			notify_start started = nullptr, notify_update update = nullptr,
			notify_finish finished = nullptr, notify_error error = nullptr);

		/*
		 *	Get list of files packed inside .evp archive
		 *
		 *	@param evp -> file path to .evp archive
		 *
		 *	@returns std::vector<FILE_PATH> -> list of files
		*/
		static LIBEVP_API std::vector<FILE_PATH> get_evp_file_list(const FILE_PATH& evp);

		/*
		 *	Unpack a single file from .evp archive into a buffer
		 *
		 *	@param evp		-> file path to .evp archive
		 *	@param file		-> file path to unpack
		 *  @param buffer	-> buffer to unpack into
		 *	@param decrypt	-> if true tries to decrypt file with the key provided (auto detects if file is encrypted)
		 *	@param key		-> decryption key (ignored if decrypt is false)
		 *
		 *	@returns evp_result
		 *			status == dv_status::ok			unpacked successfully;
		 *			status == dv_status::error		an error occurred during unpacking, msg contains details;
		*/
		static LIBEVP_API libdvsku::dv_result get_file_from_evp(const FILE_PATH& evp, const FILE_PATH& file, BUFFER& buffer,
			bool decrypt = false, const std::string& key = "");

		/*
		 *	Checks if the file is encrypted
		 * 
		 *	@param input -> file path
		*/
		static LIBEVP_API bool is_encrypted(const FILE_PATH& input);
	};
}
