#pragma once

#include <string>
#include <vector>
#include <functional>
#include <filesystem>
#include <array>

#include "utilities/dv_result.hpp"

namespace libdvsku::crypt {
	class libdvsku_crypt {
		public:
			typedef std::filesystem::path	FILE_PATH;
			typedef std::filesystem::path	FOLDER_PATH;
			typedef std::vector<uint8_t>	BUFFER;

		public:
			libdvsku_crypt(const char* key);

			// Set encryption/decryption key
			void set_key(const char* key);

			// Check if file is encrypted by checking magic
			bool is_file_encrypted(const FILE_PATH& file);

			// Check if buffer is encrypted by checking magic
			bool is_buffer_encrypted(uint8_t* buffer_ptr, size_t size);

			// Get MD5 hash
			std::array<unsigned char, 16> compute_md5(const void* ptr, size_t size);

			// Preforms encryption on the input file and saves the encrypted data to
			// the output file.
			// Encryption is skipped if file is encrypted.
			// If output is empty, result will be saved to input.
			dv_result encrypt_file(const FILE_PATH& input, const FILE_PATH& output = "");
			
			// Preforms encryption on the input file and saves the encrypted data to
			// the out buffer.
			// Encryption is skipped if file is encrypted.
			dv_result encrypt_file(const FILE_PATH& input, BUFFER& out);

			// Preforms encryption on the buffer.
			// Encryption is skipped if file is encrypted.
			// Buffer content and size will be modified.
			dv_result encrypt_buffer(BUFFER& buffer);

			// Preforms decryption on the input file and saves the data to the output file.
			// Decryption is skipped if file is not encrypted.
			// If output is empty, result will be saved to input.
			dv_result decrypt_file(const FILE_PATH& input, const FILE_PATH& output = "");
			
			// Preforms decryption on the input file and saves the data to the out buffer.
			// Decryption is skipped if file is not encrypted.
			dv_result decrypt_file(const FILE_PATH& input, BUFFER& out);

			// Preforms decryption then decompression on the buffer.
			// Buffer content and size will be modified.
			// Decryption is skipped if file is not encrypted.
			dv_result decrypt_buffer(BUFFER& buffer);

			// Preforms decryption on the buffer. Buffer data is replaced with decrypted data
			// if decryption was successful. 
			// Decryption is skipped if file is not encrypted.
			// Magic is replaced with zeroes and new_size contains the new buffer size.
			dv_result decrpyt_buffer(uint8_t* buffer_ptr, size_t size, size_t* new_size);

			// Remark: Assummes the buffer is encrypted! Only use on encrypted buffers!
			// Preforms decryption on the buffer. Buffer data is replaced with decrypted data
			// if decryption was successful.
			// Use when you know for sure that data is encrypted and DOESN'T contain magic!
			// Use when a large buffer is split into multiple smaller. 
			// Offset is offset from start of first buffer.
			// Call decrpyt_split_buffer_cleanup() after you finish decrypting.
			dv_result decrpyt_split_buffer(uint8_t* buffer_ptr, size_t size, size_t offset);

			// Cleanup box for current split buffer decryption
			void decrpyt_split_buffer_cleanup();

		private:
			std::string m_key;

		private:
			libdvsku_crypt();

			// Preforms encryption/decryption on the buffer. Buffer data is replaced with encrypted data
			// if encryption was successful.
			dv_result crypt(BUFFER& buffer);

			// Preforms encryption/decryption on the buffer. Buffer data is replaced with encrypted data
			// if encryption was successful.
			dv_result crypt(uint8_t* buffer_ptr, size_t size, size_t offset = 0, bool keep_box = false);
	};
}