#include "session/interface.h"

#include "rutil/Logger.hxx"

#include "session/sip_stack.h"

namespace rtc_session {

std::unique_ptr<StackInterface> CreateStack(const StackOptions& options) {
    auto stack = std::make_unique<SipStack>(options);
    if (!stack->Initialize()) {
        return nullptr;
    }

    return stack;
}

void SetLogger(const char *cat, const char *level, const char *filename) {
    resip::Log::initialize(cat, level, "sip_session", filename);
}
}