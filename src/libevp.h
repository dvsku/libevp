#pragma once

#ifndef LIBEVP_H
#define LIBEVP_H

#include <vector>
#include <string>
#include <functional>

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
				none,
				client_only,
				server_only
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

			typedef std::function<void()> notify_start;
			typedef std::function<void(dvsku::evp::evp_status)> notify_finish;
			typedef std::function<void(float)> notify_update;
			typedef std::function<void(dvsku::evp::evp_result)> notify_error;

		public:
			static evp_result pack(const std::string& input, const std::string& output,
				bool encrypt = false, const std::string& key = "", const file_filter& filter = file_filter::none);

			static evp_result unpack(const std::string& input, const std::string& output,
				bool decrypt = false, const std::string& key = "", const file_filter& filter = file_filter::none);

			static void pack_async(const std::string& input, const std::string& output,
				bool encrypt = false, const std::string& key = "", const file_filter& filter = file_filter::none,
				const bool* cancel = nullptr, notify_start started = nullptr, notify_update update = nullptr,
				notify_finish finished = nullptr, notify_error error = nullptr);

			static void unpack_async(const std::string& input, const std::string& output,
				bool decrypt = false, const std::string& key = "", const file_filter& filter = file_filter::none,
				const bool* cancel = nullptr, notify_start started = nullptr, notify_update update = nullptr,
				notify_finish finished = nullptr, notify_error error = nullptr);

			static bool is_encrypted(const std::string& input);

		protected:
			static evp_result pack_impl(const filesys::path& input, const filesys::path& output,
				bool encrypt = false, const std::string& key = "", const file_filter& filter = file_filter::none,
				const bool* cancel = nullptr, notify_start started = nullptr, notify_update update = nullptr,
				notify_finish finished = nullptr, notify_error error = nullptr);

			static evp_result unpack_impl(const filesys::path& input, const filesys::path& output,
				bool decrypt = false, const std::string& key = "", const file_filter& filter = file_filter::none,
				const bool* cancel = nullptr, notify_start started = nullptr, notify_update update = nullptr,
				notify_finish finished = nullptr, notify_error error = nullptr);

			static void serialize_file_desc(const file_desc& file_desc, std::vector<unsigned char>& buffer);
			
			static evp_result is_evp_header_valid(const filesys::path& input);

			static evp_result are_pack_paths_valid(const std::string& input, const std::string& output);

			static evp_result are_unpack_paths_valid(const std::string& input, const std::string& output);

			static std::vector<filesys::path> get_files(const filesys::path& dir, const file_filter& filter = file_filter::none);
	};
}

#endif
