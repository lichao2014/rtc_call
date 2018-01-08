#ifndef _RTC_CALL_USER_H_INCLUDED
#define _RTC_CALL_USER_H_INCLUDED

#include <list>

#include "api/peerconnectioninterface.h"

#include "session/interface.h"
#include "rtc_call_interface.h"

namespace rtc {

class CallEngine;

class CallUser : public CallUserInterface 
                  , public rtc_session::UserCallback
                  , public std::enable_shared_from_this<CallUser> {
public:
    static std::shared_ptr<CallUser> Create(const CallUserOptions& config,
                                            CallUserObserver *observer,
                                            const std::shared_ptr<CallEngine> call_engine);

    const CallUserOptions& options() const override { return options_; }
    const CallEngineInterface *engine() const override;
    std::shared_ptr<CallInterface> MakeCall(const std::string& peer,
                                            CallObserver *observer) override;
private:
    CallUser(const CallUserOptions& options, 
             CallUserObserver *observer,
             const std::shared_ptr<CallEngine> call_engine)
        : options_(options)
        , observer_(observer)
        , call_engine_(call_engine) {}
    bool Initialize();

    friend class Call;
    void OnLoginResult(bool success) override;
    void OnCallee(std::unique_ptr<rtc_session::CalleeInterface> callee) override;
    void OnCalleeQuit(Call *callee);

    CallUserOptions options_;
    CallUserObserver *observer_ = nullptr;
    std::shared_ptr<CallEngine> call_engine_;
    std::shared_ptr<rtc_session::UserInterface> session_user_;
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> pc_factory_;
    std::list<std::shared_ptr<Call>> callees_;
};
}

#endif // !_RTC_CALL_USER_H_INCLUDED
