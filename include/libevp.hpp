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
    enum class file_filter : unsigned int {
        none        = 0x00,   // include all files
        client_only = 0x01,   // include only Talisman Online client files
        server_only = 0x02    // include only Talisman Online server files
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
        typedef std::function<void()> start_callback_fn;

        // void f(evp_result)
        typedef std::function<void(evp_result)> finish_callback_fn;

        // void f(float)
        typedef std::function<void(float)> update_callback_fn;

        // void f(evp_result)
        typedef std::function<void(evp_result)> error_callback_fn;

        // void f(const std::filesystem::path&, std::vector<uint8_t>&)
        typedef std::function<void(const std::filesystem::path& file, std::vector<uint8_t>& buffer)> buffer_process_fn;

        typedef std::filesystem::path   FILE_PATH;
        typedef std::filesystem::path   DIR_PATH;
        typedef std::vector<uint8_t>    BUFFER;

    public:

        /*
         *  Packs files in dir into a .evp archive
         *
         *  @param input_dir    -> dir path containing files to pack
         *  @param evp          -> file path where to save the created .evp archive
         *  @param filter       ->
         *      none: packs all files in dir;
         *      client: packs only Talisman Online client related files;
         *      server: packs only Talisman Online server related files
         * 
         *  @returns evp_result    
         *      status == evp_result::e_status::ok         packed successfully;
         *      status == evp_result::e_status::error      an error occurred during packing, msg contains details;
        */
        static LIBEVP_API evp_result pack(const DIR_PATH& input_dir, const FILE_PATH& evp,
            file_filter filter = file_filter::none);

        /*
         *  Unpacks .evp archive contents into a dir
         *
         *  @param evp          -> file path to .evp archive
         *  @param output_dir   -> dir path where to save unpacked files
         *
         *  @returns evp_result
         *      status == evp_result::e_status::ok         unpacked successfully;
         *      status == evp_result::e_status::error      an error occurred during unpacking, msg contains details;
        */
        static LIBEVP_API evp_result unpack(const FILE_PATH& evp, const DIR_PATH& output_dir);

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
        static LIBEVP_API void pack_async(const DIR_PATH& input_dir, const FILE_PATH& evp, file_filter filter = file_filter::none,
            const bool* cancel = nullptr, start_callback_fn started = nullptr, update_callback_fn update = nullptr,
            finish_callback_fn finished = nullptr, error_callback_fn error = nullptr);

        /*
         *  Asynchronously unpacks .evp archive contents into a dir
         *
         *  @param evp          -> file path to .evp archive
         *  @param output_dir   -> dir path where to save unpacked files
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
        static LIBEVP_API void unpack_async(const FILE_PATH& evp, const DIR_PATH& output_dir, const bool* cancel = nullptr, 
            start_callback_fn started = nullptr, update_callback_fn update = nullptr, 
            finish_callback_fn finished = nullptr, error_callback_fn error = nullptr);

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
         *  Get filtered files
         * 
         *  @param input    -> input dir
         *  @param filter   -> filter
         * 
         *  @returns std::vector<FILE_PATH> -> filtered files
        */
        static LIBEVP_API std::vector<FILE_PATH> get_filtered_files(const DIR_PATH& input, file_filter filter);

        /*
        *   Set a handler to process a file buffer before it's packed or
        *   after it's unpacked.
        *   The handler is reset after each pack/unpack call.
        */
        static LIBEVP_API void do_buffer_processing(buffer_process_fn fn);
    };
}
