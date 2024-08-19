#pragma once

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    #if defined LIBEVP_DLL
        #define LIBEVP_API __declspec(dllexport)
    #else
        #define LIBEVP_API
    #endif
#else
    #define LIBEVP_API 
#endif

#include <libevp/evp_filter.hpp>
#include <libevp/evp_result.hpp>
#include <libevp/evp_context.hpp>
#include <libevp/evp_fd.hpp>

#include <filesystem>

namespace libevp {
    class evp {
    public:
        using dir_path_t  = std::filesystem::path;
        using file_path_t = std::filesystem::path;

    public:
        evp()           = default;
        evp(const evp&) = delete;
        evp(evp&&)      = delete;

        evp& operator=(const evp&) = delete;
        evp& operator=(evp&&)      = delete;

    public:

        /*
         *  Packs files in dir into a .evp archive
         *
         *  @param input_dir    -> dir path containing files to pack
         *  @param evp          -> file path where to save the created .evp archive
         *  @param filter       ->
         *      none:   packs all files in dir;
         *      client: packs only Talisman Online client related files;
         *      server: packs only Talisman Online server related files
         *
         *  @returns evp_result
         *      status == evp_result_status::ok         packed successfully;
         *      status == evp_result_status::error      an error occurred during packing, message contains details;
        */
        LIBEVP_API evp_result pack(const dir_path_t& input_dir, const file_path_t& evp,
            evp_filter filter = evp_filter::none);

        /*
         *  Unpacks .evp archive contents into a dir
         *
         *  @param evp          -> file path to .evp archive
         *  @param output_dir   -> dir path where to save unpacked files
         *
         *  @returns evp_result
         *      status == evp_result_status::ok         unpacked successfully;
         *      status == evp_result_status::error      an error occurred during unpacking, message contains details;
        */
        LIBEVP_API evp_result unpack(const file_path_t& evp, const dir_path_t& output_dir);

        /*
         *  Asynchronously packs files in dir into a .evp archive
         *
         *  @param input_dir    -> dir path containing files to pack
         *  @param evp          -> file path where to save the created .evp archive
         *  @param filter       ->
         *      none:   packs all files in dir;
         *      client: packs only Talisman Online client related files;
         *      server: packs only Talisman Online server related files
         *
         *  @param context -> pointer to context that has callbacks
        */
        LIBEVP_API void pack_async(const dir_path_t& input_dir, const file_path_t& evp,
            evp_filter filter = evp_filter::none, evp_context* context = nullptr);

        /*
         *  Asynchronously unpacks .evp archive contents into a dir
         *
         *  @param evp          -> file path to .evp archive
         *  @param output_dir   -> dir path where to save unpacked files
         *  @param context      -> pointer to context that has callbacks
        */
        LIBEVP_API void unpack_async(const file_path_t& evp, const dir_path_t& output_dir, evp_context* context = nullptr);

        /*
         *  Validate files packed inside .evp archive.
         *
         *  @param evp   -> file path to .evp archive
         *  @param files -> vector to store file fds that failed to validate
         *
         *  @returns evp_result
         *      status == evp_result_status::ok         all files successfully validated;
         *      status == evp_result_status::error      an error occurred, message contains details;
        */
        LIBEVP_API evp_result validate_files(const file_path_t& evp, std::vector<evp_fd>* failed_fds = nullptr);

        /*
         *  Get file fds packed inside .evp archive.
         *
         *  @param evp   -> file path to .evp archive
         *  @param files -> vector to store the file fds into
         *
         *  @returns evp_result
         *      status == evp_result_status::ok         got files successfully;
         *      status == evp_result_status::error      an error occurred, message contains details;
        */
        LIBEVP_API evp_result get_files(const file_path_t& evp, std::vector<evp_fd>& files);

        /*
         *  Unpack a single file from .evp archive into a buffer.
         *
         *  @param evp      -> file path to .evp archive
         *  @param fd       -> file fd to unpack
         *  @param buffer   -> buffer to unpack into
         *
         *  @returns evp_result
         *      status == evp_result_status::ok         unpacked successfully;
         *      status == evp_result_status::error      an error occurred during unpacking, message contains details;
        */
        LIBEVP_API evp_result get_file(const file_path_t& evp, const evp_fd& fd, std::vector<uint8_t>& buffer);

        /*
         *  Unpack a single file from .evp archive into a stringstream.
         *
         *  @param evp      -> file path to .evp archive
         *  @param fd       -> file fd to unpack
         *  @param stream   -> stream to unpack into
         *
         *  @returns evp_result
         *      status == evp_result_status::ok         unpacked successfully;
         *      status == evp_result_status::error      an error occurred during unpacking, message contains details;
        */
        LIBEVP_API evp_result get_file(const file_path_t& evp, const evp_fd& fd, std::stringstream& stream);

        /*
         *  Unpack a single file from .evp archive into a buffer.
         *
         *  @param evp      -> file path to .evp archive
         *  @param file     -> file to unpack
         *  @param buffer   -> buffer to unpack into
         *
         *  @returns evp_result
         *      status == evp_result_status::ok         unpacked successfully;
         *      status == evp_result_status::error      an error occurred during unpacking, message contains details;
        */
        LIBEVP_API evp_result get_file(const file_path_t& evp, const file_path_t& file, std::vector<uint8_t>& buffer);

        /*
         *  Unpack a single file from .evp archive into a stringstream.
         *
         *  @param evp      -> file path to .evp archive
         *  @param file     -> file to unpack
         *  @param stream   -> stream to unpack into
         *
         *  @returns evp_result
         *      status == evp_result_status::ok         unpacked successfully;
         *      status == evp_result_status::error      an error occurred during unpacking, message contains details;
        */
        LIBEVP_API evp_result get_file(const file_path_t& evp, const file_path_t& file, std::stringstream& stream);

        /*
         *  Get filtered files
         *
         *  @param input    -> input dir
         *  @param filter   -> filter
         *
         *  @returns std::vector<file_path_t> -> filtered files
        */
        LIBEVP_API std::vector<file_path_t> get_filtered_files(const dir_path_t& input, evp_filter filter);
    };
}