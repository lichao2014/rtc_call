#ifndef _RTC_SIP_STACK_H_INCLUDED
#define _RTC_SIP_STACK_H_INCLUDED

#include <list>

#include "rutil/Fifo.hxx"
#include "rutil/Mutex.hxx"
#include "rutil/Condition.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/EventStackThread.hxx"

#include "utility/callback_wrapper.h"
#include "utility/invoker.h"
#include "session/interface.h"

namespace rtc_session {

class SipUserContext;

class StackThread : public resip::EventStackThread
                  , public util::Invoker<StackThread> {
public:
    StackThread(resip::SipStack& stack, 
                resip::EventThreadInterruptor& si, 
                resip::FdPollGrp& pollGrp)
        : resip::EventStackThread(stack, si, pollGrp)
        , tasks_(&si) {}

    void Stop();

    resip::ThreadIf::Id tid() const { return mId; }
    template<typename Fn>
    void PostImpl(Fn&& fn) {
        tasks_.add(new FunctionTask<Fn>(std::forward<Fn>(fn)));
    }
private:
    void afterProcess() override;

    class TaskInterface {
    public:
        virtual ~TaskInterface() = default;
        virtual void Run() = 0;
    };

    template<typename Fn>
    class FunctionTask : public TaskInterface {
    public:
        explicit FunctionTask(Fn&& fn) : fn_(std::forward<Fn>(fn)) {}

    private:
        void Run() override { fn_(); }
        Fn fn_;
    };

    resip::Fifo<TaskInterface> tasks_;
};

class SipUserManager final {
public:
    SipUserManager() = default;

    void AddUser(std::shared_ptr<SipUserContext> user);
    void RemoveUser(std::shared_ptr<SipUserContext> user);
    void WaitAllUsersClosed();
private:
    SipUserManager(const SipUserManager&) = delete;
    SipUserManager& operator=(const SipUserManager&) = delete;

    std::list<std::shared_ptr<SipUserContext>> users_;
    resip::Mutex mu_;
    resip::Condition cond_;
};

class SipStack : public StackInterface {
public:
    explicit SipStack(const StackOptions& option) : options_(option) {}
    ~SipStack();

    bool Initialize();
    resip::SipStack& lower_stack() { return *stack_; }

    // override
    const StackOptions& options() const override { return options_; }
    std::unique_ptr<UserInterface> CreateUser(
        const UserOptions& options,
        std::shared_ptr<UserCallback> callback) override;
private:
    friend class SipUserContext;
    void OnUserDeleted(std::shared_ptr<SipUserContext> user);

    StackOptions options_;
    std::unique_ptr<resip::FdPollGrp> poll_grp_;
    std::unique_ptr<resip::EventThreadInterruptor> interruptor_;
    std::unique_ptr<StackThread> thread_;
    std::unique_ptr<resip::SipStack> stack_;

    std::unique_ptr<SipUserManager> user_manager_;
};
}

#endif // !_RTC_SIP_STACK_H_INCLUDED
