#include "session/sip_stack.h"

#include "rutil/Lock.hxx"

#include "session/sip_user.h"

using namespace rtc_session;

#define kFD_POLL_GRP_TYPE   "event"

void StackThread::Stop() {
    shutdown();
    afterProcess();
    join();
}

void StackThread::afterProcess() {
    TaskInterface *task = nullptr;
    while (task = tasks_.getNext(-1)) {
        task->Run();
        delete task;
    }
}

void SipUserManager::AddUser(std::shared_ptr<SipUserContext> user) {
    resip::Lock guard(mu_);
    users_.push_back(user);
    cond_.signal();
}

void SipUserManager::RemoveUser(std::shared_ptr<SipUserContext> user) {
    resip::Lock guard(mu_);
    users_.remove(user);
    cond_.signal();
}

void SipUserManager::WaitAllUsersClosed() {
    resip::Lock guard(mu_);
    while (!users_.empty()) {
        cond_.wait(mu_);
    }
}

SipStack::~SipStack() {
    if (user_manager_) {
        user_manager_->WaitAllUsersClosed();
    }

    if (thread_) {
        thread_->Stop();
    }

    if (stack_) {
        stack_->shutdown();
        stack_->processTimers();
    }
}

bool SipStack::Initialize() {
    poll_grp_.reset(resip::FdPollGrp::create(kFD_POLL_GRP_TYPE));
    if (!poll_grp_) {
        return false;
    }

    interruptor_ = std::make_unique<resip::EventThreadInterruptor>(*poll_grp_);
    if (!interruptor_) {
        return false;
    }

    resip::SipStackOptions options;
    options.mPollGrp = poll_grp_.get();
    options.mAsyncProcessHandler = interruptor_.get();

    stack_ = std::make_unique<resip::SipStack>(options);
    if (!stack_) {
        return false;
    }

    try {
        if (options_.udp_port) {
            stack_->addTransport(resip::UDP, *options_.udp_port);
        }

        if (options_.tcp_port.has_value()) {
            stack_->addTransport(resip::TCP, *options_.tcp_port);
        }
    } catch (...) {
        return false;
    }

    thread_ = std::make_unique<StackThread>(*stack_, *interruptor_, *poll_grp_);
    if (!thread_) {
        return false;
    }

    thread_->run();

    user_manager_.reset(new SipUserManager);

    return true;
}

std::unique_ptr<UserInterface> SipStack::CreateUser(const UserOptions& options,
                                                    std::shared_ptr<UserCallback> callback) {
    auto user_ctx = thread_->CreateShared<SipUserContext>(options, callback, *this);
    if (!user_ctx || !user_ctx->Initialize()) {
        return nullptr;
    }

    user_manager_->AddUser(user_ctx);

    return std::make_unique<SipUser>(user_ctx);
}

void SipStack::OnUserDeleted(std::shared_ptr<SipUserContext> user) {
    user_manager_->RemoveUser(user);
}