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
         *  Packs files in dir into a .evp archive.
         *
         *  @param input        -> files to pack
         *  @param evp          -> file path where to save the created .evp archive
         *  @param filter       ->
         *      none:   packs all files in dir;
         *      client: packs only Talisman Online client related files;
         *      server: packs only Talisman Online server related files
         *
         *  @returns evp_result
         *      status == evp_result_status::ok         packed successfully;
         *      status == evp_result_status::failure    an error occurred during packing, message contains details;
        */
        LIBEVP_API evp_result pack(const pack_input& input, const FILE_PATH& evp);

        /*
         *  Unpacks .evp archive contents into a dir.
         *
         *  @param evp          -> file path to .evp archive
         *  @param output_dir   -> dir path where to save unpacked files
         *  @param fds          -> (optional) fds of files to unpack
         *
         *  @returns evp_result
         *      status == evp_result_status::ok         unpacked successfully;
         *      status == evp_result_status::failure    an error occurred during unpacking, message contains details;
        */
        LIBEVP_API evp_result unpack(const unpack_input& input, const DIR_PATH& output);

        /*
         *  Asynchronously packs files in dir into a .evp archive.
         *
         *  @param input        -> files to pack
         *  @param evp          -> file path where to save the created .evp archive
         *  @param filter       ->
         *      none:   packs all files in dir;
         *      client: packs only Talisman Online client related files;
         *      server: packs only Talisman Online server related files
         *
         *  @param context -> pointer to context that has callbacks
        */
        LIBEVP_API void pack_async(const pack_input& input, const FILE_PATH& evp, evp_context* context = nullptr);

        /*
         *  Asynchronously unpacks .evp archive contents into a dir.
         *
         *  @param evp          -> file path to .evp archive
         *  @param output_dir   -> dir path where to save unpacked files
         *  @param fds          -> (optional) fds of files to unpack
         *  @param context      -> pointer to context that has callbacks
        */
        LIBEVP_API void unpack_async(const unpack_input& input, const DIR_PATH& output, evp_context* context = nullptr);

        /*
         *  Validate files packed inside .evp archive.
         *
         *  @param evp   -> file path to .evp archive
         *  @param files -> vector to store file fds that failed to validate
         *
         *  @returns evp_result
         *      status == evp_result_status::ok         all files successfully validated;
         *      status == evp_result_status::failure    an error occurred, message contains details;
        */
        LIBEVP_API evp_result validate_files(const FILE_PATH& evp, std::vector<evp_fd>* failed_fds = nullptr);

        /*
         *  Get file fds packed inside .evp archive.
         *
         *  @param evp   -> file path to .evp archive
         *  @param files -> vector to store the file fds into
         *
         *  @returns evp_result
         *      status == evp_result_status::ok         got files successfully;
         *      status == evp_result_status::failure    an error occurred, message contains details;
        */
        LIBEVP_API evp_result get_archive_fds(const FILE_PATH& evp, std::vector<evp_fd>& files);

        /*
         *  Unpack a single file from .evp archive into a buffer.
         *
         *  @param evp      -> file path to .evp archive
         *  @param fd       -> file fd to unpack
         *  @param buffer   -> buffer to unpack into
         *
         *  @returns evp_result
         *      status == evp_result_status::ok         unpacked successfully;
         *      status == evp_result_status::failure    an error occurred during unpacking, message contains details;
        */
        LIBEVP_API evp_result get_file(const FILE_PATH& evp, const evp_fd& fd, std::vector<uint8_t>& buffer);

        /*
         *  Unpack a single file from .evp archive into a stringstream.
         *
         *  @param evp      -> file path to .evp archive
         *  @param fd       -> file fd to unpack
         *  @param stream   -> stream to unpack into
         *
         *  @returns evp_result
         *      status == evp_result_status::ok         unpacked successfully;
         *      status == evp_result_status::failure    an error occurred during unpacking, message contains details;
        */
        LIBEVP_API evp_result get_file(const FILE_PATH& evp, const evp_fd& fd, std::stringstream& stream);

        /*
         *  Unpack a single file from .evp archive into a buffer.
         *
         *  @param evp      -> file path to .evp archive
         *  @param file     -> file to unpack
         *  @param buffer   -> buffer to unpack into
         *
         *  @returns evp_result
         *      status == evp_result_status::ok         unpacked successfully;
         *      status == evp_result_status::failure    an error occurred during unpacking, message contains details;
        */
        LIBEVP_API evp_result get_file(const FILE_PATH& evp, const FILE_PATH& file, std::vector<uint8_t>& buffer);

        /*
         *  Unpack a single file from .evp archive into a stringstream.
         *
         *  @param evp      -> file path to .evp archive
         *  @param file     -> file to unpack
         *  @param stream   -> stream to unpack into
         *
         *  @returns evp_result
         *      status == evp_result_status::ok         unpacked successfully;
         *      status == evp_result_status::failure    an error occurred during unpacking, message contains details;
        */
        LIBEVP_API evp_result get_file(const FILE_PATH& evp, const FILE_PATH& file, std::stringstream& stream);
    };
}
