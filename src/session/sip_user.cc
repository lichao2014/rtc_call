#include "session/sip_user.h"

#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/ClientAuthManager.hxx"
#include "resip/dum/OutgoingEvent.hxx"
#include "resip/stack/Helper.hxx"

#include "session/sip_user_state.h"
#include "session/sip_stack.h"
#include "session/sip_call.h"
#include "session/resip_util.h"
#include "session/json_contents.h"

using namespace rtc_session;

namespace {

bool ValidOptions(const UserOptions& options) {
    return !options.name.empty() && !options.realm.empty();
}

template<typename Context, typename Handle>
std::shared_ptr<Context> GetCallCtxFromHandle(Handle h) {
    auto dialog_set = dynamic_cast<SipCallDialogSet<Context> *>(
        h->getAppDialogSet().get());
    if (!dialog_set) {
        return nullptr;
    }
    return dialog_set->context();
}

bool UpdateRegisterationByRport(const resip::NameAddrs& all_contacts, 
                                const resip::Vias& vias, 
                                UpdateContacts *contacts) {
    assert(contacts);
    contacts->add.clear();
    contacts->del.clear();

    for (auto&& via : vias) {
        for (auto&& contact : all_contacts) {
            if (contact.uri().host() == via.sentHost() 
                && contact.uri().port() == via.sentPort()) {
                auto new_contact = contact;
                if (via.exists(resip::p_received)) {
                    new_contact.uri().host() = via.param(resip::p_received);
                }

                if (via.exists(resip::p_rport)) {
                    auto& rport = via.param(resip::p_rport);
                    if (rport.hasValue()) {
                        new_contact.uri().port() = rport.port();
                    }
                }

                if (!(contact == new_contact)) {
                    contacts->add.push_back(new_contact);
                    contacts->del.push_back(contact);
                }
            }
        }
    }
    return !contacts->add.empty() && !contacts->del.empty();
}

bool equal(const resip::Data& data, const std::string& str) {
    if (data.size() != str.size()) {
        return false;
    }

    return std::equal(data.begin(), data.end(), str.begin());
}
}

void SipUserContext::DumThreadWrapper::thread() {
    resip::DumThread::thread();
    ctx_->stack_.OnUserDeleted(ctx_);
}

SipUserContext::SipUserContext(const UserOptions& options, 
                               std::shared_ptr<UserCallback> callback,
                               SipStack& stack)
    : resip::DialogUsageManager(stack.lower_stack())
    , options_(options)
    , stack_(stack)
    , callback_(callback) {
}

bool SipUserContext::Initialize() {
    if (!ValidOptions(options_)) {
        return false;
    }

    resip::SharedPtr<resip::MasterProfile> master_profile(new resip::MasterProfile);
    setMasterProfile(master_profile);

    if (options_.password.has_value()) {
        master_profile->setDigestCredential(MakeData(options_.realm),
                                            MakeData(options_.name),
                                            MakeData(*options_.password));

        std::auto_ptr<resip::ClientAuthManager> client_auth_manager { 
            new resip::ClientAuthManager };
        setClientAuthManager(client_auth_manager);
    }

    master_profile->setDefaultFrom(GetDomainUserAddr(options_));
    //master_profile->setOutboundProxy(GetLoginURI(config_));
    //master_profile->setForceOutboundProxyOnAllRequestsEnabled(true);
    //master_profile->setExpressOutboundAsRouteSetEnabled(true);

    if (options_.login_keepalive_sec) {
        master_profile->setDefaultRegistrationTime(*options_.login_keepalive_sec);
    }

    master_profile->addSupportedMethod(resip::MESSAGE);
    master_profile->addSupportedMimeType(resip::MESSAGE, JsonContents::getStaticType());

    setClientRegistrationHandler(this);
    setInviteSessionHandler(this);

    std::auto_ptr<resip::AppDialogSetFactory> dialog_set_factory(
        new SipDialogSetFactory(shared_from_this()));
    setAppDialogSetFactory(dialog_set_factory);

    thread_.reset(new DumThreadWrapper(shared_from_this()));
    thread_->run();

    controller_ = std::make_unique<SipUserController>(*this);

    return true;
}

void SipUserContext::Shutdown() {
    Logout();

    Post([this] {
        shutdown(this);
    });
}

void SipUserContext::Login() {
    controller_->Register();
}

void SipUserContext::Logout() {
    controller_->Deregister();
}

void SipUserContext::SendAddRegMsg() {
    Dispatch([this] {
        send(makeRegistration(GetDomainUserAddr(options_)));
        //std::clog << "send reg msg" << std::endl;
    });
}

void SipUserContext::SendEndRegMsg() {
    Dispatch([h = client_registeration_handle_]() mutable {
        if (h.isValid()) {
            //std::clog << "send end msg" << std::endl;
            h->end();
        }
    });
}

void SipUserContext::SendUpdateRegMsg(const UpdateContacts& contacts) {
    Post([h = client_registeration_handle_, contacts = std::move(contacts)]() mutable {
        if (h.isValid()) {
            std::clog << "send remove all msg" << std::endl;
            h->removeAll();

            for (auto&& contact : contacts.add) {
                std::clog << "send binding msg" << std::endl;
                h->addBinding(contact);
            }
        }
    });
}

void SipUserContext::OnCallee(std::unique_ptr<CalleeInterface> callee) {
    callback_(&UserCallback::OnCallee, std::move(callee));
}

void SipUserContext::onDumCanBeDeleted() {
    thread_->shutdown();
}

void SipUserContext::onSuccess(resip::ClientRegistrationHandle h,
                               const resip::SipMessage& response) {
    client_registeration_handle_ = h;

    bool done = controller_->OnRegSuccess();
    if (done && options_.login_using_sip_rport
        && response.exists(resip::h_Vias)) {
        auto& vias = response.header(resip::h_Vias);

        UpdateContacts contacts;
        if (UpdateRegisterationByRport(h->allContacts(), vias, &contacts)) {
            controller_->UpdateReg(contacts);

            for (auto&& contact : contacts.add) {
                addDomain(contact.uri().host());
                getMasterUserProfile()->setOverrideHostAndPort(contact.uri());
            }

            done = false;
        }
    }

    if (done) {
        callback_(&UserCallback::OnLoginResult, true);
    }
}

void SipUserContext::onRemoved(resip::ClientRegistrationHandle,
                               const resip::SipMessage& response) {
    controller_->OnRegRemoved();
}

int SipUserContext::onRequestRetry(resip::ClientRegistrationHandle h,
                                   int retrySeconds, 
                                   const resip::SipMessage& response) {
    client_registeration_handle_ = h;
    return options_.login_retry_after_failure;
}

void SipUserContext::onFailure(resip::ClientRegistrationHandle h,
                               const resip::SipMessage& response) {
    client_registeration_handle_ = h;
    callback_(&UserCallback::OnLoginResult, false);
    controller_->OnRegFailure();
}

void SipUserContext::onNewSession(resip::ClientInviteSessionHandle h,
                                  resip::InviteSession::OfferAnswerType oat, 
                                  const resip::SipMessage& msg) {
    auto caller_ctx = GetCallCtxFromHandle<SipCallerContext>(h);
    if (caller_ctx) {
        caller_ctx->Init(h);
    }
}

void SipUserContext::onNewSession(resip::ServerInviteSessionHandle h,
                                  resip::InviteSession::OfferAnswerType oat, 
                                  const resip::SipMessage& msg) {
    auto callee_ctx = GetCallCtxFromHandle<SipCalleeContext>(h);
    if (callee_ctx) {
        callee_ctx->Init(h);
    }
}

void SipUserContext::onFailure(resip::ClientInviteSessionHandle h,
                               const resip::SipMessage& msg) {
    auto caller_ctx = GetCallCtxFromHandle<SipCallerContext>(h);
    if (caller_ctx) {
        caller_ctx->OnFailure();
    }
}

void SipUserContext::onEarlyMedia(resip::ClientInviteSessionHandle,
                                  const resip::SipMessage&, 
                                  const resip::SdpContents&) {

}

void SipUserContext::onProvisional(resip::ClientInviteSessionHandle,
                                   const resip::SipMessage&) {

}

void SipUserContext::onConnected(resip::ClientInviteSessionHandle h,
                                 const resip::SipMessage& msg) {
    auto caller_ctx = GetCallCtxFromHandle<SipCallerContext>(h);
    if (caller_ctx) {
        caller_ctx->OnConnected();
    }
}

void SipUserContext::onConnected(resip::InviteSessionHandle h,
                                 const resip::SipMessage& msg) {
    auto callee_ctx = GetCallCtxFromHandle<SipCalleeContext>(h);
    if (callee_ctx) {
        callee_ctx->OnConnected();
    }
}

void SipUserContext::onTerminated(resip::InviteSessionHandle h,
                                  resip::InviteSessionHandler::TerminatedReason reason, 
                                  const resip::SipMessage* related) {
    auto caller_ctx = GetCallCtxFromHandle<SipCallerContext>(h);
    if (caller_ctx) {
        caller_ctx->OnTerminated();
    }

    auto callee_ctx = GetCallCtxFromHandle<SipCalleeContext>(h);
    if (callee_ctx) {
        callee_ctx->OnTerminated();
    }
}

void SipUserContext::onForkDestroyed(resip::ClientInviteSessionHandle) {

}

void SipUserContext::onRedirected(resip::ClientInviteSessionHandle,
                                  const resip::SipMessage& msg) {

}

void SipUserContext::onAnswer(resip::InviteSessionHandle h,
                              const resip::SipMessage& msg, 
                              const resip::SdpContents& sdp) {
    auto caller_ctx = GetCallCtxFromHandle<SipCallerContext>(h);
    if (caller_ctx) {
        caller_ctx->OnAnswer({ sdp.getBodyData().data(), sdp.getBodyData().size() });
    }
}

void SipUserContext::onOffer(resip::InviteSessionHandle h,
                             const resip::SipMessage& msg, 
                             const resip::SdpContents& sdp) {
    auto callee_ctx = GetCallCtxFromHandle<SipCalleeContext>(h);
    if (callee_ctx) {
        callee_ctx->OnOffer({ sdp.getBodyData().data(), sdp.getBodyData().size() });
    }
}

void SipUserContext::onOfferRequired(resip::InviteSessionHandle,
                                     const resip::SipMessage& msg) {

}

void SipUserContext::onOfferRejected(resip::InviteSessionHandle,
                                     const resip::SipMessage* msg) {

}

void SipUserContext::onInfo(resip::InviteSessionHandle, 
                            const resip::SipMessage& msg) {
}

void SipUserContext::onInfoSuccess(resip::InviteSessionHandle,
                                   const resip::SipMessage& msg) {

}

void SipUserContext::onInfoFailure(resip::InviteSessionHandle,
                                   const resip::SipMessage& msg) {

}

void SipUserContext::onMessage(resip::InviteSessionHandle h,
                               const resip::SipMessage& msg) {
    auto caller_ctx = GetCallCtxFromHandle<SipCallerContext>(h);
    if (caller_ctx) {
        caller_ctx->OnMessage(MakeContents(*msg.getContents()));
    }

    auto callee_ctx = GetCallCtxFromHandle<SipCalleeContext>(h);
    if (callee_ctx) {
        callee_ctx->OnMessage(MakeContents(*msg.getContents()));
    }
}

void SipUserContext::onMessageSuccess(resip::InviteSessionHandle h,
                                      const resip::SipMessage& msg) {
    auto caller_ctx = GetCallCtxFromHandle<SipCallerContext>(h);
    if (caller_ctx) {
        caller_ctx->OnMessageResult(true);
    }

    auto callee_ctx = GetCallCtxFromHandle<SipCalleeContext>(h);
    if (callee_ctx) {
        callee_ctx->OnMessageResult(true);
    }
}

void SipUserContext::onMessageFailure(resip::InviteSessionHandle h,
                                      const resip::SipMessage& msg) {
    auto caller_ctx = GetCallCtxFromHandle<SipCallerContext>(h);
    if (caller_ctx) {
        caller_ctx->OnMessageResult(false);
    }

    auto callee_ctx = GetCallCtxFromHandle<SipCalleeContext>(h);
    if (callee_ctx) {
        callee_ctx->OnMessageResult(false);
    }
}

void SipUserContext::onRefer(resip::InviteSessionHandle,
                             resip::ServerSubscriptionHandle, 
                             const resip::SipMessage& msg) {

}

void SipUserContext::onReferNoSub(resip::InviteSessionHandle,
                                  const resip::SipMessage& msg) {

}

void SipUserContext::onReferRejected(resip::InviteSessionHandle,
                                     const resip::SipMessage& msg) {

}

void SipUserContext::onReferAccepted(resip::InviteSessionHandle,
                                     resip::ClientSubscriptionHandle, 
                                     const resip::SipMessage& msg) {

}

const StackInterface *SipUserContext::stack() const {
    return &stack_;
}

const UserOptions& SipUser::options() const {
    return ctx_->options();
}

const StackInterface *SipUser::stack() const {
    return ctx_->stack();
}

void SipUser::Login() {
    ctx_->Login();
}

void SipUser::Logout() {
    ctx_->Logout();
}

std::unique_ptr<CallerInterface> SipUser::NewCall(const UserId& peer) {
    auto caller_ctx = std::make_shared<SipCallerContext>(ctx_, peer);
    return std::make_unique<SipCaller>(caller_ctx);
}
