#include "resip/stack/PlainContents.hxx"
#include "resip/stack/SdpContents.hxx"

#include "session/interface.h"
#include "session/resip_util.h"
#include "session/json_contents.h"

namespace rtc_session {

std::unique_ptr<resip::Contents> MakeContents(
    const Contents& contents, bool shared) {
    resip::Mime mime = MakeMime(contents.type, shared);

    if (mime == resip::PlainContents::getStaticType()) {
        return std::make_unique<resip::PlainContents>(MakeData(contents.body, shared));
    }

    if (mime == resip::SdpContents::getStaticType()) {
        auto hfv = MakeHeaderFieldValue(contents.body, shared);
        return std::make_unique<resip::PlainContents>(hfv, mime);
    }

    if (mime == JsonContents::getStaticType()) {
        auto hfv = MakeHeaderFieldValue(contents.body, shared);
        return std::make_unique<JsonContents>(hfv);
    }

    return nullptr;
}
}