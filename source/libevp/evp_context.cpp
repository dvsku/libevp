#include <libevp/evp_context.hpp>

using namespace libevp;

///////////////////////////////////////////////////////////////////////////////
// PRIVATE

void evp_context::invoke_start() {
    if (start_callback)
        start_callback();
}

void evp_context::invoke_finish(const evp_result& result) {
    if (finish_callback)
        finish_callback(result);
}

void evp_context::invoke_update(float progress) {
    if (update_callback)
        update_callback(progress);
}

bool evp_context::invoke_cancel() {
    if (!cancel) return false;
    return *cancel;
}