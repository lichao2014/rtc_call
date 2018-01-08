#include "session/sip_call.h"

using namespace rtc_session;

void SipCallerContext::Invite(const std::string *offer) {
    std::weak_ptr<SipCallerContext> wp = shared_from_this();
    if (offer) {
        user_ctx_->Dispatch([wp, offer = *offer] {
            auto sp = wp.lock();
            if (sp) {
                sp->DoInvite(&offer);
            }
        });
    } else {
        user_ctx_->Dispatch([wp] {
            auto sp = wp.lock();
            if (sp) {
                sp->DoInvite(nullptr);
            }
        });
    }
}

void SipCallerContext::DoInvite(const std::string *offer) {
    resip::SdpContents contents;

    if (offer) {
        contents = MakeSdpContents(*offer);
    }

    invite_request_msg_ = user_ctx_->makeInviteSession(
        GetDomainUserAddr(user_ctx_->options(), peer().name),
        offer ? &contents : nullptr,
        new SipCallDialogSet<SipCallerContext>(shared_from_this()));

    user_ctx_->send(invite_request_msg_);
}

void SipCallerContext::End() {
    std::weak_ptr<SipCallerContext> wp = shared_from_this();
    user_ctx_->Dispatch([wp, h = h_]() mutable {
        if (h.isValid()) {
            h->end();
        } else {
            auto sp = wp.lock();
            if (sp && sp->invite_request_msg_) {
                resip::SharedPtr<resip::SipMessage> cancel_msg 
                    { resip::Helper::makeCancel(*sp->invite_request_msg_) };
                sp->user_ctx_->send(cancel_msg);
            }
        }
    });
}

void SipCalleeContext::Accept(const std::string& answer, int code) {
    user_ctx_->Dispatch([h = h_, answer, code]() mutable {
        if (h.isValid()) {
            h->provideAnswer(MakeSdpContents(answer));
            h->accept(code);
        }
    });
}

void SipCalleeContext::Reject(int code) {
    user_ctx_->Dispatch([h = h_, code]() mutable {
        if (h.isValid()) {
            h->reject(code);
        }
    });
}

resip::AppDialogSet *SipDialogSetFactory::createAppDialogSet(
    resip::DialogUsageManager& dum, const resip::SipMessage& msg) {
    if (resip::INVITE == msg.method()) {
        auto ctx = std::make_shared<SipCalleeContext>(
            user_ctx_, 
            MakeUserId(msg.header(resip::h_From)));
        user_ctx_->OnCallee(std::make_unique<SipCallee>(ctx));
        return new SipCallDialogSet<SipCalleeContext>(ctx);
    }

    return resip::AppDialogSetFactory::createAppDialogSet(dum, msg);
}
