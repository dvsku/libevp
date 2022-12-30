#pragma once

#ifndef LIBDVSKU_CRYPT
#define LIBDVSKU_CRYPT

#define WIN32_LEAN_AND_MEAN

#include <cstdint>
#include <string>
#include <vector>
#include <functional>

#ifndef _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#endif

#include <experimental/filesystem>

namespace std_filesys = std::experimental::filesystem;

#include "utilities/utilities_filesys.h"

#define CRYPT_OK				0x00
#define CRYPT_INPUT_ERROR		0x01
#define CRYPT_OUTPUT_ERROR		0x02
#define CRYPT_ERROR				0x03
#define CRYPT_NOT_ENCRYPTED		0x04
#define CRYPT_NOT_COMPRESSED	0x05
#define CRYPT_CANCELLED			0x06

#define CRYPT_MAGIC_OK			0x00
#define CRYPT_MAGIC_INVALID		0x01

#define BUF8 std::vector<uint8_t>
#define BUF8_REF std::vector<uint8_t>&
#define BUF8_REF_C const std::vector<uint8_t>&

#define KEY const char*

constexpr uint32_t HEADER[4] { 0x52D5 };

typedef uint32_t crypt_result;

namespace dvsku::crypt {
	class libdvsku_crypt {
		protected:
			struct keys {
				uint64_t m_key1;
				uint64_t m_key2;
				uint64_t m_key3;
			};

			keys m_keys{};

		public:
			// void f()
			typedef std::function<void()> notify_start;

			// void f(crypt_result)
			typedef std::function<void(crypt_result)> notify_finish;

			// void f(float)
			typedef std::function<void(float)> notify_update;

			// void f(crypt_result)
			typedef std::function<void(crypt_result)> notify_error;

		public:
			libdvsku_crypt();
			libdvsku_crypt(KEY key);
			virtual ~libdvsku_crypt();

			// Preforms encryption on the input file and saves the encrypted data to
			// the output file.
			// If output is empty, result will be saved to input.
			crypt_result encrypt_file(FILE_PATH_REF_C input, FILE_PATH_REF_C output = "");
			
			// Preforms encryption on the input file and saves the encrypted data to
			// the out buffer.
			crypt_result encrypt_file(FILE_PATH_REF_C input, BUF8_REF out);

			// Preforms encryption on every file inside folder and subfolders and
			// saves every encrypted file to the output folder with the same folder structure.
			// If output is empty, result will be saved to input.
			crypt_result encrypt_folder(FOLDER_PATH_REF_C input, FOLDER_PATH_REF_C output = "",
				FILE_FILTER_REF_C filter = FILE_FILTER_NONE);

			// Preforms encryption asynchronously on every file inside folder and subfolders and
			// saves every encrypted file to the output folder with the same folder structure.
			// If output is empty, result will be saved to input.
			void encrypt_folder_async(FOLDER_PATH_REF_C input, FOLDER_PATH_REF_C output = "", 
				FILE_FILTER_REF_C filter = FILE_FILTER_NONE, bool* cancel = nullptr, notify_start started = nullptr, 
				notify_update update = nullptr, notify_finish finished = nullptr, notify_error error = nullptr);

			// Preforms encryption on the buffer.
			// Buffer content and size will be modified.
			crypt_result encrypt_buffer(BUF8_REF buffer);

			// Preforms compression on the input file and saves the compressed data to
			// the output file.
			// If output is empty, result will be saved to input.
			crypt_result compress_file(FILE_PATH_REF_C input, FILE_PATH_REF_C output = "");

			// Preforms compression on the input file and saves the compressed data to
			// the out buffer.
			crypt_result compress_file(FILE_PATH_REF_C input, BUF8_REF out);

			// Preforms compression on every file inside folder and subfolders and
			// saves every compressed file to the output folder with the same folder structure.
			// If output is empty, result will be saved to input.
			crypt_result compress_folder(FOLDER_PATH_REF_C input, FOLDER_PATH_REF_C output = "",
				FILE_FILTER_REF_C filter = FILE_FILTER_NONE);

			// Preforms compression asynchronously on every file inside folder and subfolders and
			// saves every compressed file to the output folder with the same folder structure.
			// If output is empty, result will be saved to input.
			void compress_folder_async(FOLDER_PATH_REF_C input, FOLDER_PATH_REF_C output = "",
				FILE_FILTER_REF_C filter = FILE_FILTER_NONE, bool* cancel = nullptr, notify_start started = nullptr, 
				notify_update update = nullptr, notify_finish finished = nullptr, notify_error error = nullptr);

			// Preforms compression on the buffer.
			// Buffer content and size will be modified.
			crypt_result compress_buffer(BUF8_REF buffer);

			// Preforms compression then encryption on the input file and saves the data to
			// the output file.
			// If output is empty, result will be saved to input.
			crypt_result encrypt_and_compress_file(FILE_PATH_REF_C input, FILE_PATH_REF_C output = "");
			
			// Preforms compression then encryption on the input file and saves the data to
			// the out buffer.
			crypt_result encrypt_and_compress_file(FILE_PATH_REF_C input, BUF8_REF out);

			// Preforms compression then encryption on every file inside folder and subfolders and
			// saves every compressed and encrypted file to the output folder with the same folder structure.
			// If output is empty, result will be saved to input.
			crypt_result encrypt_and_compress_folder(FOLDER_PATH_REF_C input, FOLDER_PATH_REF_C output = "",
				FILE_FILTER_REF_C filter = FILE_FILTER_NONE);

			// Preforms compression then encryption asynchronously on every file inside folder and subfolders and
			// saves every compressed and encrypted file to the output folder with the same folder structure.
			// If output is empty, result will be saved to input.
			void encrypt_and_compress_folder_async(FOLDER_PATH_REF_C input, FOLDER_PATH_REF_C output = "", 
				FILE_FILTER_REF_C filter = FILE_FILTER_NONE, bool* cancel = nullptr, notify_start started = nullptr, 
				notify_update update = nullptr, notify_finish finished = nullptr, notify_error error = nullptr);

			// Preforms compression then encryption on the buffer.
			// Buffer content and size will be modified.
			crypt_result encrypt_and_compress_buffer(BUF8_REF buffer);

			// Preforms decryption then decompression on the input file and saves the data to
			// the output file.
			// Encryption and compression will be auto detected.
			// If output is empty, result will be saved to input.
			crypt_result decrypt_and_decompress_file(FILE_PATH_REF_C input, FILE_PATH_REF_C output = "");
			
			// Preforms decryption then decompression on the input file and saves the data to
			// the out buffer.
			// Encryption and compression will be auto detected.
			crypt_result decrypt_and_decompress_file(FILE_PATH_REF_C input, BUF8_REF out);

			// Preforms decryption then decompression on every file inside folder and subfolders and
			// saves every decompressed and decrypted file to the output folder with the same folder structure.
			// Encryption and compression will be auto detected.
			// If output is empty, result will be saved to input.
			crypt_result decrypt_and_decompress_folder(FOLDER_PATH_REF_C input, FOLDER_PATH_REF_C output = "");

			// Preforms decryption then decompression asynchronously on every file inside folder and subfolders and
			// saves every decompressed and decrypted file to the output folder with the same folder structure.
			// Encryption and compression will be auto detected.
			// If output is empty, result will be saved to input.
			void decrypt_and_decompress_folder_async(FOLDER_PATH_REF_C input, FOLDER_PATH_REF_C output = "",
				bool* cancel = nullptr, notify_start started = nullptr, notify_update update = nullptr,
				notify_finish finished = nullptr, notify_error error = nullptr);

			// Preforms decryption then decompression on the buffer.
			// Buffer content and size will be modified.
			// Encryption and compression will be auto detected.
			crypt_result decrypt_and_decompress_buffer(BUF8_REF buffer);

			bool is_buffer_encrypted(BUF8_REF_C buffer);

			void set_key(KEY key);

		protected:
			void generate_keys(KEY key);
			
			// Preforms encryption on the buffer. Buffer data is replaced with encrypted data
			// if encryption was successful.
			crypt_result encrypt(BUF8_REF buffer);

			// Preforms decryption on the buffer. Buffer data is replaced with decrypted data
			// if decryption was successful.
			crypt_result decrypt(BUF8_REF buffer);

			// Preforms compression on the buffer. Buffer data is replaced with compressed data if
			// compression was successful.
			crypt_result compress(BUF8_REF buffer);

			// Preform decompression on the buffer. Buffer data is replaced with decompressed data if
			// compression was successful.
			crypt_result decompress(BUF8_REF buffer);

			size_t get_decompressed_size(BUF8_REF_C buffer);

			crypt_result check_encryption_magic(BUF8_REF_C buffer);
			crypt_result check_compression_magic(BUF8_REF_C buffer);
	};
}

#endif
