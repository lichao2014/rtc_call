#ifndef _SIP_RESIP_UTIL_H_INCLUDED
#define _SIP_RESIP_UTIL_H_INCLUDED

#include <memory>
#include <sstream>

#include "rutil/Data.hxx"
#include "resip/stack/Mime.hxx"
#include "resip/stack/Contents.hxx"
#include "resip/stack/NameAddr.hxx"
#include "resip/stack/SdpContents.hxx"

#include "session/interface.h"

namespace rtc_session {

struct Contents;

template<typename T>
resip::Data MakeData(const T& x, bool shared = true) {
    if (shared) {
        return { resip::Data::Share, 
                    x.data(), 
                    static_cast<resip::Data::size_type>(x.size()) };
    }

    return { x.data(), static_cast<resip::Data::size_type>(x.size()) };
}

template<typename T>
inline resip::HeaderFieldValue MakeHeaderFieldValue(const T& x, 
                                                    bool shared = true) {
    resip::HeaderFieldValue hfv { x.data(), static_cast<unsigned int>(x.size()) };
    if (shared) {
        return { hfv, resip::HeaderFieldValue::NoOwnership };
    }

    return { hfv, resip::HeaderFieldValue::CopyPadding };
}

inline resip::Mime MakeMime(const std::string& mime, bool shared = true) {
    auto pos = mime.find('/');
    assert(std::string::npos != pos);

    if (shared) {
        resip::Data type(resip::Data::Share, mime.data(), static_cast<int>(pos));
        resip::Data subtype(resip::Data::Share, 
                            mime.data() + pos + 1, 
                            static_cast<int>(mime.length() - pos - 1));

        return { type, subtype };
    }

    resip::Data type(mime.data(), static_cast<int>(pos));
    resip::Data subtype(mime.data() + pos + 1, 
                        static_cast<int>(mime.length() - pos - 1));

    return { type, subtype };
}

std::unique_ptr<resip::Contents> MakeContents(
    const Contents& contents, bool shared = true);

inline Contents MakeContents(const resip::Contents& contents) {
    std::ostringstream type;
    std::ostringstream body;

    type << contents.getType();
    body << contents.getBodyData();

    return { type.str(), body.str() };
}

inline resip::SdpContents MakeSdpContents(const std::string& body) {
    return { MakeHeaderFieldValue(body), resip::SdpContents::getStaticType() };
}

inline UserId MakeUserId(const resip::NameAddr& from) {
    std::string realm(from.uri().host().data(), from.uri().host().size());
    std::string name(from.uri().user().data(), from.uri().user().size());
    return { std::move(realm), std::move(name) };
}

inline resip::NameAddr GetDomainUserAddr(const UserOptions& options, const std::string& name) {
    resip::Uri uri;

    uri.scheme() = resip::Symbols::Sip;
    uri.user() = name.c_str();
    uri.host() = options.realm.c_str();
    uri.port() = options.login_server_port;

    return resip::NameAddr(uri);
}

inline resip::NameAddr GetDomainUserAddr(const UserOptions& options) {
    return GetDomainUserAddr(options, options.name);
}
}

#endif // !_SIP_RESIP_UTIL_H_INCLUDED
