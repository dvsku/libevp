#pragma once

#ifndef LIBDVSKU_CRYPT
#define LIBDVSKU_CRYPT

#define WIN32_LEAN_AND_MEAN

#include <cstdint>
#include <string>
#include <vector>

#define CRYPT_OK 0
#define CRYPT_INPUT_ERROR 1
#define CRYPT_OUTPUT_ERROR 2
#define CRYPT_ERROR 3
#define CRYPT_NOT_ENCRYPTED 4
#define CRYPT_NOT_COMPRESSED 5

#define CRYPT_MAGIC_OK 0
#define CRYPT_MAGIC_INVALID 1

#define BUF8 std::vector<uint8_t>
#define BUF8_REF std::vector<uint8_t>&
#define BUF8_REF_C const std::vector<uint8_t>&

#define FILE_PATH const char*
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
			libdvsku_crypt(KEY key);
			virtual ~libdvsku_crypt();

			// Preforms encryption on the input file and saves the encrypted data to
			// the output file.
			crypt_result encrypt_file(FILE_PATH input, FILE_PATH output);
			
			// Preforms encryption on the input file and saves the encrypted data to
			// the out buffer.
			crypt_result encrypt_file(FILE_PATH input, BUF8_REF out);

			// Preforms encryption on the buffer.
			// Buffer content and size will be modified.
			crypt_result encrypt_buffer(BUF8_REF buffer);

			// Preforms compression on the input file and saves the compressed data to
			// the output file.
			crypt_result compress_file(FILE_PATH input, FILE_PATH output);

			// Preforms compression on the input file and saves the compressed data to
			// the out buffer.
			crypt_result compress_file(FILE_PATH input, BUF8_REF out);

			// Preforms compression on the buffer.
			// Buffer content and size will be modified.
			crypt_result compress_buffer(BUF8_REF buffer);

			// Preforms compression then encryption on the input file and saves the data to
			// the output file.
			crypt_result encrypt_and_compress_file(FILE_PATH input, FILE_PATH output);
			
			// Preforms compression then encryption on the input file and saves the data to
			// the out buffer.
			crypt_result encrypt_and_compress_file(FILE_PATH input, BUF8_REF out);

			// Preforms compression then encryption on the buffer.
			// Buffer content and size will be modified.
			crypt_result encrypt_and_compress_buffer(BUF8_REF buffer);

			// Preforms decryption then decompression on the input file and saves the data to
			// the output file.
			// Encryption and compression will be auto detected.
			crypt_result decrypt_and_decompress_file(FILE_PATH input, FILE_PATH output);
			
			// Preforms decryption then decompression on the input file and saves the data to
			// the out buffer.
			// Encryption and compression will be auto detected.
			crypt_result decrypt_and_decompress_file(FILE_PATH input, BUF8_REF out);

			// Preforms decryption then decompression on the buffer.
			// Buffer content and size will be modified.
			// Encryption and compression will be auto detected.
			crypt_result decrypt_and_decompress_buffer(BUF8_REF buffer);

			bool is_buffer_encrypted(BUF8_REF_C buffer);

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
