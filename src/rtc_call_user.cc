#include "rtc_call_user.h"
#include "rtc_call_engine.h"
#include "rtc_call.h"

using namespace rtc;

//static 
std::shared_ptr<CallUser> 
CallUser::Create(const CallUserOptions& options,
                 CallUserObserver *observer,
                 const std::shared_ptr<CallEngine> call_engine) {
    std::shared_ptr<CallUser> user(new CallUser(options, observer, call_engine));
    if (!user->Initialize()) {
        return nullptr;
    }
    return user;
}

bool CallUser::Initialize() {
    rtc_session::UserOptions options;
    options.name = options_.name;
    options.realm = options_.domain;
    options.login_server_port = options_.login_server_port;
    options.login_using_sip_rport = call_engine_->options().session.login_using_sip_rport;
    options.password.emplace(options_.password);

    session_user_ = call_engine_->CreateSessionUser(options, shared_from_this());
    if (!session_user_) {
        return false;
    }

    session_user_->Login();

    pc_factory_ = call_engine_->CreatePeerConnectionFactory();
    if (!pc_factory_) {
        return false;
    }

    return true;
}

const CallEngineInterface *CallUser::engine() const {
    return call_engine_.get();
}

std::shared_ptr<CallInterface> 
CallUser::MakeCall(const std::string& peer, CallObserver *observer) {
    rtc_session::UserId callee_id;
    callee_id.name = peer;
    callee_id.realm = options_.domain;

    return Call::CreateCaller(*this, 
                              pc_factory_, 
                              session_user_->NewCall(callee_id),
                              observer);
}

void CallUser::OnLoginResult(bool success) {
    observer_->OnLogin(success);
}

void CallUser::OnCallee(std::unique_ptr<rtc_session::CalleeInterface> callee) {
    auto call_callee = Call::CreateCallee(*this, pc_factory_, std::move(callee));
    callees_.push_back(call_callee);
    call_callee->SetObserver(observer_->OnCallee(call_callee));
}

void CallUser::OnCalleeQuit(Call *callee) {
    callees_.remove_if([&] (auto &i) {
        return i.get() == callee;
    });
}
