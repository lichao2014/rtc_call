#ifndef _RTC_JSON_CONTENTS_H_INCLUDED
#define _RTC_JSON_CONTENTS_H_INCLUDED

#include "resip/stack/Contents.hxx"

#include "json/json.h"

namespace rtc_session {

class JsonContents : public resip::Contents {
public:
    Json::Value root;

    JsonContents() : resip::Contents(getStaticType()) {}

    explicit JsonContents(const resip::HeaderFieldValue& hfv)
        : resip::Contents(hfv, getStaticType()) {}

    JsonContents(const char* field, unsigned int fieldLength)
        : JsonContents({ field, fieldLength }) {}

    static const resip::Mime& getStaticType() {
        static resip::Mime _type("application", "json");
        return _type;
    }
private:
    explicit JsonContents(const Json::Value& root)
        : resip::Contents(getStaticType())
        , root(root) {}

    EncodeStream& encodeParsed(EncodeStream& str) const override;
    void parse(resip::ParseBuffer& pb) override;

    resip::Contents *clone() const override {
        return new JsonContents(*this);
    }
};
}

#endif // !_RTC_JSON_CONTENTS_H_INCLUDED
