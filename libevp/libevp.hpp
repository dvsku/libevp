#pragma once

#include <vector>
#include <string>
#include <functional>
#include <filesystem>

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
    enum class file_filter {
        none,           // include all files
        client_only,    // include only Talisman Online client files
        server_only     // include only Talisman Online server files
    };

    struct evp_result {
        enum class e_status : unsigned char {
            ok        = 0x0,
            cancelled = 0x1,
            error     = 0x2
        };

        evp_result::e_status status = evp_result::e_status::ok;
        std::string msg = "";

        evp_result() {};

        evp_result(evp_result::e_status _status)
            : status(_status) {};

        evp_result(evp_result::e_status _status, const std::string& _msg)
            : status(_status), msg(_msg) {};

        explicit operator bool() const {
            return status == evp_result::e_status::ok;
        }
    };

    class evp {
    public:
        // void f()
        typedef std::function<void()> notify_start;

        // void f(evp_result)
        typedef std::function<void(evp_result)> notify_finish;

        // void f(float)
        typedef std::function<void(float)> notify_update;

        // void f(evp_result)
        typedef std::function<void(evp_result)> notify_error;

        // void f(const std::filesystem::path&, std::vector<uint8_t>&)
        typedef std::function<void(const std::filesystem::path& file, std::vector<uint8_t>& buffer)> buffer_process_fn;

        typedef std::filesystem::path   FILE_PATH;
        typedef std::filesystem::path   FOLDER_PATH;
        typedef std::vector<uint8_t>    BUFFER;

    public:

        /*
         *  Packs files in folder into a .evp archive
         *
         *  @param input    -> folder path containing files to pack
         *  @param output   -> file path where to save the created .evp archive
         *  @param filter   ->
         *      none: packs all files in folder;
         *      client: packs only Talisman Online client related files;
         *      server: packs only Talisman Online server related files
         * 
         *  @returns evp_result    
         *      status == evp_result::e_status::ok         packed successfully;
         *      status == evp_result::e_status::error      an error occurred during packing, msg contains details;
        */
        static LIBEVP_API evp_result pack(const FOLDER_PATH& input_dir, const FILE_PATH& evp,
            file_filter filter = file_filter::none);

        /*
         *  Unpacks .evp archive contents into a folder
         *
         *  @param input    -> file path to .evp archive
         *  @param output   -> folder path where to save unpacked files
         *
         *  @returns evp_result
         *      status == evp_result::e_status::ok         unpacked successfully;
         *      status == evp_result::e_status::error      an error occurred during unpacking, msg contains details;
        */
        static LIBEVP_API evp_result unpack(const FILE_PATH& evp, const FOLDER_PATH& output_dir);

        /*
         *  Asynchronously packs files in folder into a .evp archive
         *
         *  @param input    -> folder path containing files to pack
         *  @param output   -> file path where to save the created .evp archive
         *  @param filter   ->
         *      none:   packs all files in folder;
         *      client: packs only Talisman Online client related files;
         *      server: packs only Talisman Online server related files
         *  
         *  @param cancel   -> pointer to bool that cancels packing if value is true
         *  @param started  -> callback that's called when packing starts
         *  @param update   -> callback that's called when there's progress update
         *  @param finished -> callback that's called when packing ends
         *  @param error    -> callback that's called when an error occurres
         *
         *  @returns evp_result
         *      status == evp_result::e_status::ok             packed successfully;
         *      status == evp_result::e_status::error          an error occurred during packing, msg contains details;
         *      status == evp_result::e_status::cancelled      packing cancelled by user
        */
        static LIBEVP_API void pack_async(const FOLDER_PATH& input_dir, const FILE_PATH& evp, file_filter filter = file_filter::none,
            const bool* cancel = nullptr, notify_start started = nullptr, notify_update update = nullptr,
            notify_finish finished = nullptr, notify_error error = nullptr);

        /*
         *  Asynchronously unpacks .evp archive contents into a folder
         *
         *  @param evp          -> file path to .evp archive
         *  @param output_dir   -> folder path where to save unpacked files
         *  @param cancel       -> pointer to bool that cancels unpacking if value is true
         *  @param started      -> callback that's called when unpacking starts
         *  @param update       -> callback that's called when there's progress update
         *  @param finished     -> callback that's called when unpacking ends
         *  @param error        -> callback that's called when an error occurres
         * 
         *  @returns evp_result
         *      status == evp_result::e_status::ok             unpacked successfully;
         *      status == evp_result::e_status::error          an error occurred during unpacking, msg contains details;
         *      status == evp_result::e_status::cancelled      unpacking cancelled by user
        */
        static LIBEVP_API void unpack_async(const FILE_PATH& evp, const FOLDER_PATH& output_dir, const bool* cancel = nullptr, 
            notify_start started = nullptr, notify_update update = nullptr, 
            notify_finish finished = nullptr, notify_error error = nullptr);

        /*
         *  Get list of files packed inside .evp archive
         *
         *  @param evp -> file path to .evp archive
         *
         *  @returns std::vector<FILE_PATH> -> list of files
        */
        static LIBEVP_API std::vector<FILE_PATH> get_evp_file_list(const FILE_PATH& evp);

        /*
         *  Unpack a single file from .evp archive into a buffer
         *
         *  @param evp      -> file path to .evp archive
         *  @param file     -> file path to unpack
         *  @param buffer   -> buffer to unpack into
         *
         *  @returns evp_result
         *      status == evp_result::e_status::ok         unpacked successfully;
         *      status == evp_result::e_status::error      an error occurred during unpacking, msg contains details;
        */
        static LIBEVP_API evp_result get_file_from_evp(const FILE_PATH& evp, const FILE_PATH& file, BUFFER& buffer);

        /*
         *  Unpack a single file from .evp archive into a stringstream
         *
         *  @param evp      -> file path to .evp archive
         *  @param file     -> file path to unpack
         *  @param stream   -> stream to unpack into
         *
         *  @returns evp_result
         *      status == evp_result::e_status::ok         unpacked successfully;
         *      status == evp_result::e_status::error      an error occurred during unpacking, msg contains details;
        */
        static LIBEVP_API evp_result get_file_from_evp(const FILE_PATH& evp, const FILE_PATH& file, std::stringstream& stream);

        /*
        *   Set a handler to process a file buffer before it's packed or
        *   after it's unpacked.
        *   The handler is reset after each pack/unpack call.
        */
        static LIBEVP_API void do_buffer_processing(buffer_process_fn fn);
    };
}
