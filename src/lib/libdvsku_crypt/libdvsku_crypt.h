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
#define CRYPT_ENCRYPTED			0x05
#define CRYPT_CANCELLED			0x06

// unsigned char vector buffer
#define BUFV8 std::vector<uint8_t>

// unsigned char vector buffer reference
#define BUFV8_REF std::vector<uint8_t>&

// const unsigned char vector buffer reference
#define BUFV8_REF_C const std::vector<uint8_t>&

// unsigned char pointer
#define BUF8 uint8_t*

#define KEY const char*

typedef uint32_t crypt_result;

namespace dvsku::crypt {
	class libdvsku_crypt {
		protected:
			

			std::string m_key;

			libdvsku_crypt();

		public:
			struct box {
				uint32_t m_index_A = 0;
				uint32_t m_index_B = 0;
				uint8_t m_data[256] = {};
			};

			// void f()
			typedef std::function<void()> notify_start;

			// void f(crypt_result)
			typedef std::function<void(crypt_result)> notify_finish;

			// void f(float)
			typedef std::function<void(float)> notify_update;

			// void f(crypt_result)
			typedef std::function<void(crypt_result)> notify_error;

		public:
			libdvsku_crypt(KEY key);
			virtual ~libdvsku_crypt();

			// Set encryption/decryption key
			void set_key(KEY key);

			// Check if file is encrypted by checking magic
			bool is_file_encrypted(FILE_PATH_REF_C file);

			// Write magic to file
			void write_magic_file(FILE_PATH_REF_C file);

			// Remove magic from file
			void remove_magic_file(FILE_PATH_REF_C file);

			// Preforms encryption on the input file and saves the encrypted data to
			// the output file.
			// Encryption is skipped if file is encrypted.
			// If output is empty, result will be saved to input.
			crypt_result encrypt_file(FILE_PATH_REF_C input, FILE_PATH_REF_C output = "");
			
			// Preforms encryption on the input file and saves the encrypted data to
			// the out buffer.
			// Encryption is skipped if file is encrypted.
			crypt_result encrypt_file(FILE_PATH_REF_C input, BUFV8_REF out);

			// Preforms encryption on every file inside folder and subfolders and
			// saves every encrypted file to the output folder with the same folder structure.
			// Encryption is skipped if file is encrypted.
			// If output is empty, result will be saved to input.
			crypt_result encrypt_folder(FOLDER_PATH_REF_C input, FOLDER_PATH_REF_C output = "",
				FILE_FILTER_REF_C filter = FILE_FILTER_NONE);

			// Preforms encryption asynchronously on every file inside folder and subfolders and
			// saves every encrypted file to the output folder with the same folder structure.
			// Encryption is skipped if file is encrypted.
			// If output is empty, result will be saved to input.
			void encrypt_folder_async(FOLDER_PATH_REF_C input, FOLDER_PATH_REF_C output = "", 
				FILE_FILTER_REF_C filter = FILE_FILTER_NONE, bool* cancel = nullptr, notify_start started = nullptr, 
				notify_update update = nullptr, notify_finish finished = nullptr, notify_error error = nullptr);

			// Preforms encryption on the buffer.
			// Encryption is skipped if file is encrypted.
			// Buffer content and size will be modified.
			crypt_result encrypt_buffer(BUFV8_REF buffer);

			// Preforms decryption on the input file and saves the data to the output file.
			// Decryption is skipped if file is not encrypted.
			// If output is empty, result will be saved to input.
			crypt_result decrypt_file(FILE_PATH_REF_C input, FILE_PATH_REF_C output = "");
			
			// Preforms decryption on the input file and saves the data to the out buffer.
			// Decryption is skipped if file is not encrypted.
			crypt_result decrypt_file(FILE_PATH_REF_C input, BUFV8_REF out);

			// Preforms decryption on every file inside folder and subfolders and
			// saves every decrypted file to the output folder with the same folder structure.
			// Decryption is skipped if file is not encrypted.
			// If output is empty, result will be saved to input.
			crypt_result decrypt_folder(FOLDER_PATH_REF_C input, FOLDER_PATH_REF_C output = "");

			// Preforms decryption asynchronously on every file inside folder and subfolders and
			// saves every decrypted file to the output folder with the same folder structure.
			// Decryption is skipped if file is not encrypted.
			// If output is empty, result will be saved to input.
			void decrypt_folder_async(FOLDER_PATH_REF_C input, FOLDER_PATH_REF_C output = "",
				bool* cancel = nullptr, notify_start started = nullptr, notify_update update = nullptr,
				notify_finish finished = nullptr, notify_error error = nullptr);

			// Check if buffer is encrypted by checking magic
			bool is_buffer_encrypted(BUF8 buffer, uint64_t size);

			// Preforms decryption then decompression on the buffer.
			// Buffer content and size will be modified.
			// Decryption is skipped if file is not encrypted.
			crypt_result decrypt_buffer(BUFV8_REF buffer);

			// Preforms decryption on the buffer. Buffer data is replaced with decrypted data
			// if decryption was successful. 
			// Decryption is skipped if file is not encrypted.
			// Magic is replaced with zeroes and new_size contains the new buffer size.
			crypt_result decrpyt_buffer(BUF8 buffer, uint64_t size, uint64_t* new_size);

			// Remark: Assummes the buffer is encrypted! Only use on encrypted buffers!
			// Preforms decryption on the buffer. Buffer data is replaced with decrypted data
			// if decryption was successful.
			// Use when you know for sure that data is encrypted and DOESN'T contain magic!
			// Useful when reading file block by block for example.
			crypt_result decrpyt_buffer_unsafe(BUF8 buffer, uint64_t size);

		protected:
			dvsku::crypt::libdvsku_crypt::box generate_box();
			
			// Preforms encryption/decryption on the buffer. Buffer data is replaced with encrypted data
			// if encryption was successful.
			crypt_result crypt(BUFV8_REF buffer);

			// Preforms encryption/decryption on the buffer. Buffer data is replaced with encrypted data
			// if encryption was successful.
			crypt_result crypt(BUF8 buffer, uint64_t size);

			// Check if buffer has magic
			bool has_magic(BUFV8_REF_C buffer);

			// Check if buffer has magic
			bool has_magic(BUF8 buffer, uint64_t size);

			// Writes magic to buffer. Buffer size is modified to fit the magic.
			void write_magic(BUFV8_REF buffer);

			// Writes magic to buffer. Buffer must have space available for it or it will overwrite existing data.
			void write_magic(BUF8 buffer, uint64_t size);

			// Removes magic from buffer. Buffer size is modified when magic is removed.
			void remove_magic(BUFV8_REF buffer);

			// Removes magic from buffer. Magic data is set to zeroes.
			// User must handle the new size (smaller by 4 bytes).
			void remove_magic(BUF8 buffer, uint64_t size);
	};
}

#endif
