#include "session/json_contents.h"

#include "json/writer.h"
#include "json/reader.h"

using namespace rtc_session;

EncodeStream& JsonContents::encodeParsed(EncodeStream& str) const {
    return str << root;
}

void JsonContents::parse(resip::ParseBuffer& pb) {
    const char* begin = pb.position();
    const char* end = pb.skipToEnd();

    Json::Reader reader;
    if (!reader.parse(begin, end, root)) {
        pb.fail(__FILE__, __LINE__, "json parse failed");
    }
}