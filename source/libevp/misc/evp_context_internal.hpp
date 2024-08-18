#pragma once

#include "libevp/evp_context.hpp"

namespace libevp {
    class evp_context_internal {
    public:
        evp_context_internal() = delete;
        evp_context_internal(evp_context* context);

    public:
        void invoke_start() const;
        void invoke_finish(evp_result& result) const;
        void invoke_update(float change) const;

        bool is_cancelled() const;
        void invoke_cancel() const;

    private:
        evp_context* m_context;
    };
}
