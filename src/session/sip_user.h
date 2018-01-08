#ifndef _RTC_SIP_USER_H_INCLUDED
#define _RTC_SIP_USER_H_INCLUDED

#include <type_traits>

#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/DumThread.hxx"
#include "resip/dum/DumShutdownHandler.hxx"
#include "resip/dum/DumCommand.hxx"
#include "resip/dum/DumFeature.hxx"
#include "resip/dum/ClientRegistration.hxx"
#include "resip/dum/RegistrationHandler.hxx"
#include "resip/dum/InviteSessionHandler.hxx"
#include "resip/dum/ServerInviteSession.hxx"

#include "utility/invoker.h"
#include "utility/callback_wrapper.h"
#include "session/interface.h"

namespace rtc_session {

class SipStack;
class SipUser;
class SipUserController;
struct UpdateContacts;

template<typename Fn, bool Copy = std::is_copy_constructible_v<Fn>>
class FunctionDumCommand : public resip::DumCommandAdapter {
public:
    explicit FunctionDumCommand(Fn&& fn) : fn_(std::forward<Fn>(fn)) {}
private:
    void executeCommand() override {
        fn_();
    }

    resip::Message* clone() const override {
        auto fn = fn_;
        return new FunctionDumCommand<Fn>(std::move(fn));
    }

    EncodeStream& encodeBrief(EncodeStream& strm) const override {
        return strm << "FunctionDumCommand";
    }

    Fn fn_;
};

template<typename Fn>
class FunctionDumCommand<Fn, false> : public FunctionDumCommand<Fn, true> {
public:
    using FunctionDumCommand<Fn, true>::FunctionDumCommand;
private:
    resip::Message* clone() const override {
        return nullptr;
    }
};

template<typename Fn>
decltype(auto) MakeFunctionDumCommand(Fn&& fn) {
    return std::make_unique<FunctionDumCommand<Fn>>(std::forward<Fn>(fn));
}

class SipUserContext : public resip::DialogUsageManager
                     , public resip::ClientRegistrationHandler
                     , public resip::InviteSessionHandler
                     , public resip::DumShutdownHandler
                     , public util::Invoker<SipUserContext>
                     , public std::enable_shared_from_this<SipUserContext> {
public:
    SipUserContext(const UserOptions& config, 
                   std::shared_ptr<UserCallback> callback, SipStack& stack);

    bool Initialize();
    void Shutdown();

    const StackInterface *stack() const;
    const UserOptions& options() const { return options_; }
    void Login();
    void Logout();

    // impl dum's invoker
    resip::ThreadIf::Id tid() const { return thread_->id(); }
    template<typename Fn>
    void PostImpl(Fn&& fn) {
        post(MakeFunctionDumCommand(std::forward<Fn>(fn)).release());
    }
private:
    friend class SipUserRegisteringState;
    friend class SipUserRegisteredState;
    friend class SipUserDeregisteringState;
    friend class SipUserDeregisteredState;
    friend class SipUserUpdatingState;
    friend class SipDialogSetFactory;

    void SendAddRegMsg();
    void SendEndRegMsg();
    void SendUpdateRegMsg(const UpdateContacts& contacts);

    friend class SipDialogSetFactory;
    void OnCallee(std::unique_ptr<CalleeInterface> callee);

    void onDumCanBeDeleted() override;

    // client registeration handler
    /// Called when registraion succeeds or each time it is sucessfully
    /// refreshed (manual refreshes only). 
    void onSuccess(resip::ClientRegistrationHandle, 
                  const resip::SipMessage& response) override;

    // Called when all of my bindings have been removed
    void onRemoved(resip::ClientRegistrationHandle, 
                   const resip::SipMessage& response)override;

    /// call on Retry-After failure. 
    /// return values: -1 = fail, 0 = retry immediately, N = retry in N seconds
    int onRequestRetry(resip::ClientRegistrationHandle, 
                       int retrySeconds, 
                       const resip::SipMessage& response) override;

    /// Called if registration fails, usage will be destroyed (unless a 
    /// Registration retry interval is enabled in the Profile)
    void onFailure(resip::ClientRegistrationHandle, 
                   const resip::SipMessage& response) override;

    // invite handler
    /// called when an initial INVITE or the intial response to an outoing invite  
    void onNewSession(resip::ClientInviteSessionHandle, 
                      resip::InviteSession::OfferAnswerType oat, 
                      const resip::SipMessage& msg) override;
    void onNewSession(resip::ServerInviteSessionHandle,
                      resip::InviteSession::OfferAnswerType oat, 
                      const resip::SipMessage& msg) override;

    /// Received a failure response from UAS
    void onFailure(resip::ClientInviteSessionHandle, 
                   const resip::SipMessage& msg) override;

    /// called when an in-dialog provisional response is received that contains a body
    void onEarlyMedia(resip::ClientInviteSessionHandle, 
                      const resip::SipMessage&, 
                      const resip::SdpContents&) override;

    /// called when dialog enters the Early state - typically after getting 18x
    void onProvisional(resip::ClientInviteSessionHandle, 
                       const resip::SipMessage&) override;

    /// called when a dialog initiated as a UAC enters the connected state
    void onConnected(resip::ClientInviteSessionHandle, 
                     const resip::SipMessage& msg) override;

    /// called when a dialog initiated as a UAS enters the connected state
    void onConnected(resip::InviteSessionHandle, 
                     const resip::SipMessage& msg) override;

    void onTerminated(resip::InviteSessionHandle, 
                      InviteSessionHandler::TerminatedReason reason, 
                      const resip::SipMessage* related = 0) override;

    /// called when a fork that was created through a 1xx never receives a 2xx
    /// because another fork answered and this fork was canceled by a proxy. 
    void onForkDestroyed(resip::ClientInviteSessionHandle) override;

    /// called when a 3xx with valid targets is encountered in an early dialog     
    /// This is different then getting a 3xx in onTerminated, as another
    /// request will be attempted, so the DialogSet will not be destroyed.
    /// Basically an onTermintated that conveys more information.
    /// checking for 3xx respones in onTerminated will not work as there may
    /// be no valid targets.
    void onRedirected(resip::ClientInviteSessionHandle, 
                      const resip::SipMessage& msg) override;
    /// called when an answer is received - has nothing to do with user
    /// answering the call 
    void onAnswer(resip::InviteSessionHandle, 
                  const resip::SipMessage& msg, 
                  const resip::SdpContents&) override;

    /// called when an offer is received - must send an answer soon after this
    void onOffer(resip::InviteSessionHandle, 
                 const resip::SipMessage& msg, 
                 const resip::SdpContents&) override;

    /// called when an Invite w/out offer is sent, or any other context which
    /// requires an offer from the user
    void onOfferRequired(resip::InviteSessionHandle, 
                         const resip::SipMessage& msg) override;

    /// called if an offer in a UPDATE or re-INVITE was rejected - not real
    /// useful. A SipMessage is provided if one is available
    void onOfferRejected(resip::InviteSessionHandle, 
                        const resip::SipMessage* msg) override;

    /// called when INFO message is received 
    /// the application must call acceptNIT() or rejectNIT()
    /// once it is ready for another message.
    void onInfo(resip::InviteSessionHandle, 
                const resip::SipMessage& msg) override;

    /// called when response to INFO message is received 
    void onInfoSuccess(resip::InviteSessionHandle, 
                      const resip::SipMessage& msg) override;
    void onInfoFailure(resip::InviteSessionHandle, 
                       const resip::SipMessage& msg) override;

    /// called when MESSAGE message is received 
    void onMessage(resip::InviteSessionHandle, 
                   const resip::SipMessage& msg) override;

    /// called when response to MESSAGE message is received 
    void onMessageSuccess(resip::InviteSessionHandle, 
                          const resip::SipMessage& msg) override;
    void onMessageFailure(resip::InviteSessionHandle, 
                          const resip::SipMessage& msg) override;

    /// called when an REFER message is received.  The refer is accepted or
    /// rejected using the server subscription. If the offer is accepted,
    /// DialogUsageManager::makeInviteSessionFromRefer can be used to create an
    /// InviteSession that will send notify messages using the ServerSubscription
    void onRefer(resip::InviteSessionHandle, 
                 resip::ServerSubscriptionHandle, 
                 const resip::SipMessage& msg) override;

    void onReferNoSub(resip::InviteSessionHandle, 
                      const resip::SipMessage& msg) override;

    /// called when an REFER message receives a failure response 
    void onReferRejected(resip::InviteSessionHandle, 
                         const resip::SipMessage& msg) override;

    /// called when an REFER message receives an accepted response 
    void onReferAccepted(resip::InviteSessionHandle, 
                         resip::ClientSubscriptionHandle, 
                         const resip::SipMessage& msg) override;

    class DumThreadWrapper : public resip::DumThread {
    public:
        explicit DumThreadWrapper(std::shared_ptr<SipUserContext> ctx)
            : resip::DumThread(*ctx)
            , ctx_(ctx) {}
        Id id() const { return mId; }

    private:
        void thread() override;
        std::shared_ptr<SipUserContext> ctx_;
    };

    UserOptions options_;
    util::CallbackWrapper<UserCallback> callback_;
    SipStack& stack_;
    std::unique_ptr<DumThreadWrapper> thread_;
    std::unique_ptr<SipUserController> controller_;
    resip::ClientRegistrationHandle client_registeration_handle_;
};

class SipUser : public UserInterface {
public:
    explicit SipUser(std::shared_ptr<SipUserContext> ctx) : ctx_(ctx) {}
    ~SipUser() { ctx_->Shutdown(); }

    auto& context() { return ctx_; }

    // override
    const UserOptions& options() const override;
    const StackInterface *stack() const override;
    void Login() override;
    void Logout() override;
    std::unique_ptr<CallerInterface> NewCall(const UserId& peer) override;
private:
    std::shared_ptr<SipUserContext> ctx_;
};
}

#endif // !_RTC_SIP_USER_H_INCLUDED
