#ifndef _RTC_SIP_USER_STATE_H_INCLUDED
#define _RTC_SIP_USER_STATE_H_INCLUDED

#include <assert.h>

#include "rutil/RecursiveMutex.hxx"
#include "rutil/Lock.hxx"
#include "resip/stack/NameAddr.hxx"

#include "utility/singleton.h"

namespace rtc_session {

class SipUserContext;
class SipUserController;

struct UpdateContacts {
    resip::NameAddrs add;
    resip::NameAddrs del;
};

class SipUserStateInterface {
protected:
    virtual ~SipUserStateInterface() = default;
public:
    // op
    virtual void DoRegisteration(SipUserController *) = 0;
    virtual void DoDeregisteration(SipUserController *) = 0;
    virtual void DoUpdateReg(SipUserController *, const UpdateContacts&) = 0;
    virtual bool OnRegSuccess(SipUserController *) = 0;
    virtual void OnRegFailure(SipUserController *) = 0;
    virtual void OnRegRemoved(SipUserController *) = 0;
    virtual const char *name() const = 0;
};

class SipUserController {
public:
    explicit SipUserController(SipUserContext& ctx);

    void Register();
    void Deregister();
    void UpdateReg(const UpdateContacts& contacts);
    bool OnRegSuccess();
    void OnRegFailure();
    void OnRegRemoved();
private:
    friend class SipUserDeregisteredState;
    friend class SipUserRegisteringState;
    friend class SipUserRegisteredState;
    friend class SipUserDeregisteringState;
    friend class SipUserUpdatingState;

    void set_state(SipUserStateInterface &state) { 
        std::clog << "set user state:" 
            << state_->name() << "->" << state.name() << std::endl;
        state_ = &state;
    }
    template<typename S>
    void set_state() { set_state(S::Instance()); }

    mutable resip::RecursiveMutex mu_;
    SipUserContext& ctx_ ;
    SipUserStateInterface *state_ = nullptr;
    bool regist_op_ = false;
};

template<typename T>
class SipUserState : public util::Singleton<T>
                   , public SipUserStateInterface {
protected:
    void DoRegisteration(SipUserController *) override {}
    void DoDeregisteration(SipUserController *) override {}
    void DoUpdateReg(SipUserController *, const UpdateContacts&) override {}
    bool OnRegSuccess(SipUserController *) override { return true; }
    void OnRegFailure(SipUserController *) override {}
    void OnRegRemoved(SipUserController *) override {}
};

class SipUserDeregisteredState : public SipUserState<SipUserDeregisteredState> {
private:
    void DoRegisteration(SipUserController *controller) override;
    const char *name() const override { return "Deregistered";  }
};

class SipUserRegisteringState : public SipUserState<SipUserRegisteringState> {
private:
    bool OnRegSuccess(SipUserController *controller) override;
    void OnRegFailure(SipUserController *controller) override;
    const char *name() const override { return "Registering"; }
};

class SipUserRegisteredState : public SipUserState<SipUserRegisteredState> {
private:
    void DoDeregisteration(SipUserController *controller) override;
    void DoUpdateReg(SipUserController *, const UpdateContacts&) override;
    bool OnRegSuccess(SipUserController *controller) override;
    void OnRegFailure(SipUserController *controller) override;
    const char *name() const override { return "Registered"; }
};

class SipUserDeregisteringState : public SipUserState<SipUserDeregisteringState> {
private:
    void OnRegRemoved(SipUserController *controller) override;
    const char *name() const override { return "Deregistering"; }
};

class SipUserUpdatingState : public SipUserState<SipUserUpdatingState> {
private:
    bool OnRegSuccess(SipUserController *controller) override;
    void OnRegFailure(SipUserController *controller) override;
    const char *name() const override { return "Updating"; }
};
}

#endif // !_RTC_SIP_USER_STATE_H_INCLUDED
