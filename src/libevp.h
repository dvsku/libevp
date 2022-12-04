#pragma once

#ifndef LIBEVP_H
#define LIBEVP_H

#include <string>

#include "definitions.h"

namespace dvsku {
	class evp {
		protected:
			struct file_desc {
				std::string m_path;
				BUF8 m_data;
				unsigned char m_data_hash[16];
				size_t m_data_size;
				size_t m_path_size;
				size_t m_data_start_offset;
			};

			enum file_filter : unsigned int {
				none,
				client_only,
				server_only
			};

			struct evp_result {
				evp_status m_code = evp_status::ok;
				std::string m_msg = "";

				evp_result() {}
				evp_result(evp_status code) : m_code(code) {}
				evp_result(evp_status code, const std::string& msg) : m_code(code), m_msg(msg) {}
			};

			typedef void (*notify_fn)(float);
			typedef void (*error_fn)(evp_result);

		public:
			static evp_result pack(const std::string& input, const std::string& output,
				bool encrypt = false, const std::string& key = "", const file_filter& filter = file_filter::none,
				notify_fn started = nullptr, notify_fn progress_update = nullptr,
				notify_fn finished = nullptr, error_fn error = nullptr);

			static evp_result unpack(const std::string& input, const std::string& output,
				bool decrypt = false, const std::string& key = "", const file_filter& filter = file_filter::none,
				notify_fn started = nullptr, notify_fn progress_update = nullptr,
				notify_fn finished = nullptr, error_fn error = nullptr);

			static void pack_async(const std::string& input, const std::string& output,
				bool encrypt = false, const std::string& key = "", const file_filter& filter = file_filter::none,
				notify_fn started = nullptr, notify_fn progress_update = nullptr, notify_fn finished = nullptr,
				error_fn error = nullptr);

			static void unpack_async(const std::string& input, const std::string& output,
				bool decrypt = false, const std::string& key = "", const file_filter& filter = file_filter::none,
				notify_fn started = nullptr, notify_fn progress_update = nullptr, notify_fn finished = nullptr,
				error_fn error = nullptr);

			static bool is_encrypted(const std::string& input);

		protected:
			static evp_result pack_impl(const filesys::path& input, const filesys::path& output,
				bool encrypt = false, const std::string& key = "", const file_filter& filter = file_filter::none,
				notify_fn started = nullptr, notify_fn progress_update = nullptr, notify_fn finished = nullptr,
				error_fn error = nullptr);

			static evp_result unpack_impl(const filesys::path& input, const filesys::path& output,
				bool decrypt = false, const std::string& key = "", const file_filter& filter = file_filter::none,
				notify_fn started = nullptr, notify_fn progress_update = nullptr, notify_fn finished = nullptr,
				error_fn error = nullptr);

			static void serialize_file_desc(const file_desc& file_desc, std::vector<unsigned char>& buffer);
			
			static evp_result is_evp_header_valid(const filesys::path& input);

			static evp_result are_pack_paths_valid(const std::string& input, const std::string& output);

			static evp_result are_unpack_paths_valid(const std::string& input, const std::string& output);

			static std::vector<filesys::path> get_files(const filesys::path& dir, const file_filter& filter = file_filter::none);
	};
}

#endif
