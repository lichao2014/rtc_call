#ifndef _RTC_SESSION_INTERFACE_H_INCLUDED
#define _RTC_SESSION_INTERFACE_H_INCLUDED

#include <memory>
#include <string>

#include "utility/optional.h"

namespace rtc_session {

class StackInterface;
class UserInterface;
class CallInterface;
class CallerInterface;
class CalleeInterface;

struct Contents {
    std::string type;
    std::string body;
};

struct UserId {
    std::string realm;
    std::string name;
};

class CallCallback {
protected:
    virtual ~CallCallback() = default;
public:
    virtual void OnInit() = 0;
    virtual void OnFailure() = 0;
    virtual void OnOffer(const std::string& offer) = 0;
    virtual void OnAnswer(const std::string& answer) = 0;
    virtual void OnMessage(const Contents& msg) = 0;
    virtual void OnMessageResult(bool success) = 0;
    virtual void OnConnected() = 0;
    virtual void OnTerminated() = 0;
};

class CallInterface {
public:
    virtual ~CallInterface() = default;
    virtual const UserId& peer() const = 0;
    virtual bool GetLocalSdp(std::string *out) = 0;
    virtual void Message(const Contents& msg) = 0;
    virtual void AcceptNIT(int code = 200, const Contents *msg = nullptr) = 0;
    virtual void RejectNIT(int code = 488) = 0;
    virtual void SetCallback(std::shared_ptr<CallCallback> callback) = 0;
};

class CalleeInterface : public CallInterface {
public:
    virtual ~CalleeInterface() = default;
    virtual void Accept(const std::string& answer, int code = 200) = 0;
    virtual void Reject(int code = 488) = 0;
};

class CallerInterface : public CallInterface {
public:
    virtual ~CallerInterface() = default;
    virtual void Invite(const std::string *offer) = 0;
};

struct UserOptions : UserId {
    util::Optional<std::string> password;
    uint16_t login_server_port = 0;
    bool login_using_sip_rport = true;
    util::Optional<uint32_t> login_keepalive_sec;
    int login_retry_after_failure = -1;
};

class UserCallback {
protected:
    virtual ~UserCallback() = default;
public:
    virtual void OnLoginResult(bool success) = 0;
    virtual void OnCallee(std::unique_ptr<CalleeInterface> callee) = 0;
};

class UserInterface {
public:
    virtual ~UserInterface() = default;
    virtual const UserOptions& options() const = 0;
    virtual const StackInterface *stack() const = 0;
    virtual void Login() = 0;
    virtual void Logout() = 0;
    virtual std::unique_ptr<CallerInterface> NewCall(const UserId& peer) = 0;
};

struct StackOptions {
    util::Optional<uint16_t> udp_port;
    util::Optional<uint16_t> tcp_port;
    bool compression = false;

    StackOptions() = default;
    explicit StackOptions(uint16_t udp_port) : udp_port(udp_port) {}
};

class StackInterface {
public:
    virtual ~StackInterface() = default;
    virtual const StackOptions& options() const = 0;
    virtual std::unique_ptr<UserInterface> CreateUser(const UserOptions& options, 
                                                      std::shared_ptr<UserCallback> callback) = 0;
};

std::unique_ptr<StackInterface> CreateStack(const StackOptions& options);
void SetLogger(const char *cat, const char *level, const char *filename);
}

#endif // !_RTC_SESSION_INTERFACE_H_INCLUDED
