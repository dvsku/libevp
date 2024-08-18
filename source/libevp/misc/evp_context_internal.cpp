#include "evp_context_internal.hpp"

using namespace libevp;

evp_context_internal::evp_context_internal(evp_context* context)
    : m_context(context) {}

void evp_context_internal::invoke_start() const {
    if (m_context && m_context->start_callback)
        m_context->start_callback();
}

void evp_context_internal::invoke_finish(evp_result& result) const {
    if (m_context && m_context->finish_callback)
        m_context->finish_callback(result);
}

void evp_context_internal::invoke_update(float change) const {
    if (m_context && m_context->update_callback)
        m_context->update_callback(change);
}

bool evp_context_internal::is_cancelled() const {
    if (!m_context || !m_context->cancel) return false;
    return m_context->cancel->load();
}

void evp_context_internal::invoke_cancel() const {
    evp_result result;
    result.status = evp_result::status::cancelled;

    invoke_finish(result);
}
