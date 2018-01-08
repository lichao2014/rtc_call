#ifndef _RTC_CALL_INTERFACE_H_INCLUED
#define _RTC_CALL_INTERFACE_H_INCLUED

#include <memory>
#include <vector>
#include <string>
#include <cstdint>

#include "rtc_common_types.h"

namespace rtc {

class CallInterface;
class CallUserInterface;
class CallEngineInterface;

class CallObserver {
protected:
    virtual ~CallObserver() = default;
public:
    virtual void OnError() = 0;
    virtual std::unique_ptr<I420VideoSinkInterface> OnAddStream(
        bool remote, const std::string&stream_label, const std::string&track_id) = 0;
};

class CallInterface {
protected:
    virtual ~CallInterface() = default;
public:
    virtual const CallUserInterface *user() const = 0;
    virtual const std::string& peer() const = 0;
};

class CallUserObserver {
protected:
    virtual ~CallUserObserver() = default;
public:
    virtual void OnLogin(bool ok) = 0;
    virtual CallObserver *OnCallee(std::shared_ptr<CallInterface> callee) = 0;
};

struct CallUserOptions {
    std::string domain;
    std::string name;
    std::string password;
    uint16_t login_server_port = 0;
};

class CallUserInterface {
protected:
    virtual ~CallUserInterface() = default;
public:
    virtual const CallUserOptions& options() const = 0;
    virtual const CallEngineInterface *engine() const = 0;
    virtual std::shared_ptr<CallInterface> MakeCall(const std::string& peer,
                                                    CallObserver *observer) = 0;
};

struct IceServer {
    std::vector<std::string> urls;
    std::string username;
    std::string password;
};

struct CallEngineOptions {
    struct Session {
        uint16_t udp_port = 0;
        uint16_t tcp_port = 0;
        uint32_t login_keepalive_sec = 0;
        bool compression = false;
        bool login_using_sip_rport = true;
    } session;

    std::vector<IceServer> ice_servers;
};

class CallEngineInterface {
protected:
    virtual ~CallEngineInterface() = default;
public:
    virtual const CallEngineOptions& options() const = 0;
    virtual std::shared_ptr<CallUserInterface> CreateUser(const CallUserOptions& options,
                                                          CallUserObserver *observer) = 0;
};

std::shared_ptr<CallEngineInterface> CreateCallEngine(const CallEngineOptions& options);
}

#endif // !_RTC_CALL_INTERFACE_H_INCLUED
