#pragma once

#include <libevp/evp_defs.hpp>
#include <libevp/evp_context.hpp>
#include <libevp/model/evp_fd.hpp>
#include <libevp/model/evp_result.hpp>

#include <vector>

namespace libevp {
    class evp {
    public:
        struct pack_input {
            DIR_PATH              base;
            std::vector<DIR_PATH> files;
        };

        struct unpack_input {
            FILE_PATH           archive;
            std::vector<evp_fd> files;
        };

    public:
        evp()           = default;
        evp(const evp&) = delete;
        evp(evp&&)      = delete;

        evp& operator=(const evp&) = delete;
        evp& operator=(evp&&)      = delete;

    public:

        /*
         *  Pack files in dir into an archive.
         *
         *  @param input    -> files to pack
         *  @param output   -> file path where to save the created archive
         *
         *  @returns evp_result
         *      status == evp_result_status::ok         packed successfully;
         *      status == evp_result_status::failure    an error occurred during packing, message contains details;
        */
        LIBEVP_API evp_result pack(const pack_input& input, const FILE_PATH& output);

        /*
         *  Unpack archive contents into a dir.
         *
         *  @param input    -> archive and optional files
         *  @param output   -> dir path where to save unpacked files
         *
         *  @returns evp_result
         *      status == evp_result_status::ok         unpacked successfully;
         *      status == evp_result_status::failure    an error occurred during unpacking, message contains details;
        */
        LIBEVP_API evp_result unpack(const unpack_input& input, const DIR_PATH& output);

        /*
         *  Asynchronously pack files in dir into an archive.
         *
         *  @param input    -> files to pack
         *  @param output   -> file path where to save the created .evp archive
         *  @param context  -> pointer to context that has callbacks
        */
        LIBEVP_API void pack_async(const pack_input& input, const FILE_PATH& output, evp_context* context = nullptr);

        /*
         *  Asynchronously unpack archive contents into a dir.
         *
         *  @param input    -> archive and optional files
         *  @param output   -> dir path where to save unpacked files
         *  @param context  -> pointer to context that has callbacks
        */
        LIBEVP_API void unpack_async(const unpack_input& input, const DIR_PATH& output, evp_context* context = nullptr);

        /*
         *  Validate files packed inside archive.
         *
         *  @param input    -> file path to archive
         *  @param files    -> vector to store file fds that failed to validate
         *
         *  @returns evp_result
         *      status == evp_result_status::ok         all files successfully validated;
         *      status == evp_result_status::failure    an error occurred, message contains details;
        */
        LIBEVP_API evp_result validate_files(const FILE_PATH& input, std::vector<evp_fd>* failed_files = nullptr);

        /*
         *  Get file fds packed inside archive.
         *
         *  @param input    -> file path to archive
         *  @param files    -> vector to store the file fds into
         *
         *  @returns evp_result
         *      status == evp_result_status::ok         got files successfully;
         *      status == evp_result_status::failure    an error occurred, message contains details;
        */
        LIBEVP_API evp_result get_archive_fds(const FILE_PATH& input, std::vector<evp_fd>& files);

        /*
         *  Unpack a single file from archive into a buffer.
         *
         *  @param input    -> file path to archive
         *  @param file     -> file fd to unpack
         *  @param buffer   -> buffer to unpack into
         *
         *  @returns evp_result
         *      status == evp_result_status::ok         unpacked successfully;
         *      status == evp_result_status::failure    an error occurred during unpacking, message contains details;
        */
        LIBEVP_API evp_result get_file(const FILE_PATH& input, const evp_fd& file, std::vector<uint8_t>& buffer);

        /*
         *  Unpack a single file from archive into a stringstream.
         *
         *  @param input    -> file path to archive
         *  @param file     -> file fd to unpack
         *  @param stream   -> stream to unpack into
         *
         *  @returns evp_result
         *      status == evp_result_status::ok         unpacked successfully;
         *      status == evp_result_status::failure    an error occurred during unpacking, message contains details;
        */
        LIBEVP_API evp_result get_file(const FILE_PATH& input, const evp_fd& file, std::stringstream& stream);

        /*
         *  Unpack a single file from archive into a buffer.
         *
         *  @param input    -> file path to archive
         *  @param file     -> file to unpack
         *  @param buffer   -> buffer to unpack into
         *
         *  @returns evp_result
         *      status == evp_result_status::ok         unpacked successfully;
         *      status == evp_result_status::failure    an error occurred during unpacking, message contains details;
        */
        LIBEVP_API evp_result get_file(const FILE_PATH& input, const FILE_PATH& file, std::vector<uint8_t>& buffer);

        /*
         *  Unpack a single file from archive into a stringstream.
         *
         *  @param input    -> file path to archive
         *  @param file     -> file to unpack
         *  @param stream   -> stream to unpack into
         *
         *  @returns evp_result
         *      status == evp_result_status::ok         unpacked successfully;
         *      status == evp_result_status::failure    an error occurred during unpacking, message contains details;
        */
        LIBEVP_API evp_result get_file(const FILE_PATH& input, const FILE_PATH& file, std::stringstream& stream);
    };
}
