#ifndef _RTC_CALL_ENGINE_H_INCLUDED
#define _RTC_CALL_ENGINE_H_INCLUDED

#include <vector>

#include "rtc_base/thread.h"
#include "api/peerconnectioninterface.h"

#include "session/interface.h"
#include "rtc_call_interface.h"

namespace rtc {

class CallEngine : public CallEngineInterface
                 , public std::enable_shared_from_this<CallEngine> {
public:
    static std::shared_ptr<CallEngine> Create(const CallEngineOptions& config);

    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> 
        CreatePeerConnectionFactory();

    std::unique_ptr<rtc_session::UserInterface> CreateSessionUser(
        const rtc_session::UserOptions& options,  std::shared_ptr<rtc_session::UserCallback> callback);

    const CallEngineOptions& options() const override { return options_; }
    std::shared_ptr<CallUserInterface> CreateUser(const CallUserOptions& options,
                                                  CallUserObserver *observer) override;
private:
    explicit CallEngine(const CallEngineOptions& options) : options_(options) {};
    bool Initialize();

    CallEngineOptions options_;
    std::unique_ptr<rtc_session::StackInterface> session_stack_;
    std::unique_ptr<rtc::Thread> network_thread_;
    std::unique_ptr<rtc::Thread> worker_thread_;
    std::unique_ptr<rtc::Thread> signaling_thread_;
};
}

#endif // !_RTC_CALL_ENGINE_H_INCLUDED
