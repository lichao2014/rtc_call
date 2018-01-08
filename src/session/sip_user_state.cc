#include "session/sip_user_state.h"
#include "session/sip_user.h"

using namespace rtc_session;

SipUserController::SipUserController(SipUserContext& ctx)
    : ctx_(ctx)
    , state_(&SipUserDeregisteredState::Instance()){
}

void SipUserController::Register() {
    resip::Lock guard(mu_);
    state_->DoRegisteration(this);
    regist_op_ = true;

    std::clog << "op:register" << std::endl;
}

void SipUserController::Deregister() {
    resip::Lock guard(mu_);
    state_->DoDeregisteration(this);
    regist_op_ = false;

    std::clog << "op:deregister" << std::endl;
}

void SipUserController::UpdateReg(const UpdateContacts& contacts) {
    resip::Lock guard(mu_);
    state_->DoUpdateReg(this, contacts);

    std::clog << "op:update_reg" << std::endl;
}

bool SipUserController::OnRegSuccess() {
    resip::Lock guard(mu_);

    std::clog << "op:on_reg_success" << std::endl;
    return state_->OnRegSuccess(this);
}

void SipUserController::OnRegFailure() {
    resip::Lock guard(mu_);
    state_->OnRegFailure(this);

    std::clog << "op:on_reg_failure" << std::endl;
}

void SipUserController::OnRegRemoved() {
    resip::Lock guard(mu_);
    state_->OnRegRemoved(this);

    std::clog << "op:on_reg_removed" << std::endl;
}

void SipUserDeregisteredState::DoRegisteration(SipUserController *controller) {
    controller->ctx_.SendAddRegMsg();
    controller->set_state<SipUserRegisteringState>();
}

bool SipUserRegisteringState::OnRegSuccess(SipUserController *controller) {
    controller->set_state<SipUserRegisteredState>();

    if (!controller->regist_op_) {
        controller->Deregister();
    }

    return controller->regist_op_;
}

void SipUserRegisteringState::OnRegFailure(SipUserController *controller) {
    controller->set_state<SipUserDeregisteredState>();
}

void SipUserRegisteredState::DoDeregisteration(SipUserController *controller) {
    controller->ctx_.SendEndRegMsg();
    controller->set_state<SipUserDeregisteringState>();
}

void SipUserRegisteredState::DoUpdateReg(SipUserController *controller, 
                                         const UpdateContacts& contacts) {
    controller->ctx_.SendUpdateRegMsg(contacts);
    controller->set_state<SipUserUpdatingState>();
}

bool SipUserRegisteredState::OnRegSuccess(SipUserController *controller) {
    return controller->regist_op_;
}

void SipUserRegisteredState::OnRegFailure(SipUserController *controller) {
    controller->set_state<SipUserDeregisteredState>();
}

void SipUserDeregisteringState::OnRegRemoved(SipUserController *controller) {
    controller->set_state<SipUserDeregisteredState>();

    if (controller->regist_op_) {
        controller->Register();
    }
}

bool SipUserUpdatingState::OnRegSuccess(SipUserController *controller) {
    controller->set_state<SipUserRegisteredState>();

    if (!controller->regist_op_) {
        controller->Deregister();
    }

    return controller->regist_op_;
}

void SipUserUpdatingState::OnRegFailure(SipUserController *controller) {
    controller->set_state<SipUserDeregisteredState>();
}
