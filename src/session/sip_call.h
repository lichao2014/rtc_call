#ifndef _RTC_SIP_SESSION_CALL_H_INCLUDED
#define _RTC_SIP_SESSION_CALL_H_INCLUDED

#include "resip/dum/AppDialogSet.hxx"
#include "resip/dum/ClientInviteSession.hxx"
#include "resip/dum/ServerInviteSession.hxx"
#include "resip/dum/AppDialogSetFactory.hxx"

#include "session/sip_user.h"
#include "session/resip_util.h"

namespace rtc_session {

template<typename Handle = resip::InviteSessionHandle>
class SipCallContext {
public:
    SipCallContext(std::shared_ptr<SipUserContext> user_ctx, const UserId& user_id)
        : user_ctx_(user_ctx)
        , user_id_(user_id) {
        //std::clog << "call ctx new\t" << this << std::endl;
    }

    ~SipCallContext() {
        //std::clog << "call ctx release\t" << this << std::endl;
    }

    auto& user_context() { return user_ctx_; }
    const UserId& peer() const { return user_id_; }
    void SetCallback(std::shared_ptr<CallCallback> callback) { 
        callback_ = callback; 
    }

    bool GetLocalSdp(std::string *out) {
        return user_ctx_->Invoke([this, out] {
            if (!h_.isValid()) {
                return false;
            }

            if (!h_->hasLocalSdp()) {
                return false;
            }

            auto& body = h_->getLocalSdp().getBodyData();
            out->assign(body.data(), body.size());

            return true;
        });
    }

    void Message(const Contents& msg) {
        user_ctx_->Dispatch([h = h_, msg]() mutable {
            if (h.isValid()) {
                h->message(*MakeContents(msg));
            }
        });
    }

    void AcceptNIT(int code, const Contents *msg) {
        if (msg) {
            user_ctx_->Dispatch([h = h_, code, msg = *msg]() mutable {
                if (h.isValid()) {
                    h->acceptNIT(code, MakeContents(msg).get());
                }
            });
        } else {
            user_ctx_->Dispatch([h = h_, code]() mutable {
                if (h.isValid()) {
                    h->acceptNIT(code);
                }
            });
        }
    }

    void RejectNIT(int code) {
        user_ctx_->Dispatch([h = h_, code]() mutable {
            if (h.isValid()) {
                h->rejectNIT(code);
            }
        });
    }

    void End() {
        user_ctx_->Dispatch([h = h_]() mutable {
            if (h.isValid()) {
                h->end();
            }
        });
    }

    void Init(Handle h) {
        h_ = h;
        callback_(&CallCallback::OnInit);
    }

    void OnFailure() {
        callback_(&CallCallback::OnFailure);
    }

    void OnOffer(const std::string& offer) {
        callback_(&CallCallback::OnOffer, offer);
    }

    void OnAnswer(const std::string& answer) {
        callback_(&CallCallback::OnAnswer, answer);
    }

    void OnMessage(const Contents& msg) {
        callback_(&CallCallback::OnMessage, msg);
    }

    void OnMessageResult(bool success) {
        callback_(&CallCallback::OnMessageResult, success);
    }

    void OnConnected() {
        callback_(&CallCallback::OnConnected);
    }

    void OnTerminated() {
        callback_(&CallCallback::OnTerminated);
    }

protected:
    Handle h_;
    UserId user_id_;
    std::shared_ptr<SipUserContext> user_ctx_;
    util::CallbackWrapper<CallCallback> callback_;
};

class SipCallerContext 
    : public SipCallContext<resip::ClientInviteSessionHandle>
    , public std::enable_shared_from_this<SipCallerContext> {
public:
    using SipCallContext<resip::ClientInviteSessionHandle>::SipCallContext;

    void Invite(const std::string *offer);
    void End();
private:
    void DoInvite(const std::string *offer);

    resip::SharedPtr<resip::SipMessage> invite_request_msg_;
};

class SipCalleeContext 
    : public SipCallContext<resip::ServerInviteSessionHandle>
    , public std::enable_shared_from_this<SipCalleeContext> {
public:
    using SipCallContext<resip::ServerInviteSessionHandle>::SipCallContext;

    void Accept(const std::string& answer, int code);
    void Reject(int code);
};

template<typename T = SessionCallInterface, typename Context = SipCallContext<>>
class SipCall : public T {
public:
    explicit SipCall(std::shared_ptr<Context> ctx) : ctx_(ctx) {}

    ~SipCall() override { ctx_->End(); }

    const UserId& peer() const override {
        return ctx_->peer();
    }

    bool GetLocalSdp(std::string *out) override {
        return ctx_->GetLocalSdp(out);
    }

    void Message(const Contents& msg) override {
        ctx_->Message(msg);
    }

    void AcceptNIT(int code, const Contents *msg) override {
        ctx_->AcceptNIT(code, msg);
    }

    void RejectNIT(int code) override {
        ctx_->RejectNIT(code);
    }

    void SetCallback(std::shared_ptr<CallCallback> callback) override {
        ctx_->SetCallback(callback);
    }
protected:
    std::shared_ptr<Context> ctx_;
};

class SipCaller : public SipCall<CallerInterface, SipCallerContext> {
public:
    using SipCall<CallerInterface, SipCallerContext>::SipCall;

    void Invite(const std::string *offer) override {
        ctx_->Invite(offer);
    }
};

class SipCallee : public SipCall<CalleeInterface, SipCalleeContext> {
public:
    using SipCall<CalleeInterface, SipCalleeContext>::SipCall;

    void Accept(const std::string& answer, int code) override {
        ctx_->Accept(answer, code);
    }

    void Reject(int code) override {
        ctx_->Reject(code);
    }
};

template<typename Context>
class SipCallDialogSet : public resip::AppDialogSet {
public:
    explicit SipCallDialogSet(std::shared_ptr<Context> ctx)
        : resip::AppDialogSet(*ctx->user_context())
        , ctx_(ctx) {}

    const std::shared_ptr<Context>& context() const { return ctx_; }
private:
    std::shared_ptr<Context> ctx_;
};

class SipDialogSetFactory : public resip::AppDialogSetFactory {
public:
    explicit SipDialogSetFactory(std::shared_ptr<SipUserContext> user_ctx)
        : user_ctx_(user_ctx) {}
private:
    resip::AppDialogSet* createAppDialogSet(resip::DialogUsageManager&, 
                                            const resip::SipMessage&) override;

    std::shared_ptr<SipUserContext> user_ctx_;
};
}

#endif // !_RTC_SIP_SESSION_CALL_H_INCLUDED
