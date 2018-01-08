#include <iostream>
#include <future>
#include <thread>
#include <vector>
#include <fstream>
#include <cassert>

#include "session/interface.h"

//#include "rtc_base/flags.h"

namespace {

std::string ReadAll(const std::string& filename) {
    std::ifstream in(filename, std::ios::binary);
    in.seekg(0, std::ios::end);
    auto filelen = in.tellg();
    in.seekg(0);

    std::string ret(filelen, 0);
    in.read(&ret[0], filelen);
    return ret;
}
}

class TestEnv {
public:
    explicit TestEnv(uint16_t udp_port) {
        rtc_session::StackOptions options(udp_port);
        stack_ = rtc_session::CreateStack(options);
    }

    const std::string& login_server_host() const {
        return login_server_host_;
    }

    uint16_t login_server_port() const {
        return login_server_port_;
    }

    const std::string& pwd() const {
        return password_;
    }

    std::string offer_sdp() const {
        return ReadAll("sdp.txt");
    }

    rtc_session::StackInterface *stack() {
        return stack_.get();
    }

private:
    std::string login_server_host_ { "101.132.33.178" };
    std::string password_ { "123456" };
    uint16_t login_server_port_ = 3390;

    std::unique_ptr<rtc_session::StackInterface> stack_;
};

class UserAgent : public rtc_session::CallCallback
                , public rtc_session::UserCallback
                , public std::enable_shared_from_this<UserAgent> {
public:
    explicit UserAgent(TestEnv *env) : env_(env) {}

    void Login(const std::string& name) {
        if (!user_) {
            rtc_session::UserOptions options;
            options.name = name;
            options.realm = env_->login_server_host();
            options.password = env_->pwd();
            options.login_server_port = env_->login_server_port();
            options.login_keepalive_sec = 60;

            user_ = env_->stack()->CreateUser(options, shared_from_this());
        }

        user_->Login();
    }

    void Logout() {
        if (user_) {
            user_->Logout();
        }
    }

    void Call(const std::string& peer) {
        callee_uid_.name = peer;
        callee_uid_.realm = env_->login_server_host();
    }

    ~UserAgent() {
        std::clog << "ua " << this  << " release\t"  << std::endl;
    }

private:
    void OnLoginResult(bool success) override {
        std::clog << user_->options().name << " login " 
            << (success ? "success" : "failure") << std::endl;

        if (!success) {
            return;
        }

        if (!callee_uid_.name.empty()) {
            caller_ = user_->NewCall(callee_uid_);
            caller_->SetCallback(shared_from_this());

            auto sdp = env_->offer_sdp();
            caller_->Invite(&sdp);
        }
    }

    void OnCallee(std::unique_ptr<rtc_session::CalleeInterface> callee) override {
        callee_ = std::move(callee);
        callee_->SetCallback(shared_from_this());
    }

    void OnFailure() override {
        std::clog << "on_failure\t" << call()->peer().name << std::endl;
    }

    void OnOffer(const std::string& offer) override {
        std::clog << "on_offer\t" << call()->peer().name << "\t" << offer << std::endl;

        callee_->Accept(offer);
        callee_->Message({"application/json", "{ \"name\" : 123}"});
    }

    void OnAnswer(const std::string& answer) override {
        std::clog << "on_answer\t" << call()->peer().name << "\t" << answer << std::endl;

        call()->Message({"application/json", "{ \"name\" : 123}"});
    }

    void OnMessage(const rtc_session::Contents& msg) override {
        std::clog << "on_message\t" << call()->peer().name << "\t" << msg.body << std::endl;

        call()->AcceptNIT();
    }
    void OnMessageResult(bool success) override {
        std::clog << "on_message_result\t" << call()->peer().name << "\t" << success << std::endl;
    }

    void OnConnected() override {
        std::clog << "on_connected" << std::endl;
    }

    void OnTerminated() override {
        std::clog << "on_end" << std::endl;
    }

    rtc_session::CallInterface *call(bool *is_callee = nullptr) {
        if (caller_) {
            //*is_callee = false;
            return caller_.get();
        }

        if (callee_) {
            //*is_callee = true;
            return callee_.get();
        }

        return nullptr;
    }

    TestEnv *env_ = nullptr;
    rtc_session::UserId callee_uid_;
    std::unique_ptr<rtc_session::UserInterface> user_;
    std::unique_ptr<rtc_session::CallerInterface> caller_;
    std::unique_ptr<rtc_session::CalleeInterface> callee_;
};

/*
DEFINE_string(log, "rtc_session_test.log", "log file");
DEFINE_int(port, 0, "sip port");
DEFINE_string(name, NULL, "my name");
DEFINE_string(peer, NULL, "peer name");


int main(int argc, char *argv[]) {
    /
    if (rtc::FlagList::SetFlagsFromCommandLine(&argc, argv, false) < 0) {
        rtc::FlagList::Print(nullptr, true);
        return -1;
    }

    rtc_session::SetLogger("file", "STACK", FLAG_log);

    TestEnv env(FLAG_port);
    auto ua = std::make_shared<UserAgent>(&env);

    ua->Login(FLAG_name);
    if (FLAG_peer) {
        ua->Call(FLAG_peer);
    }
 
    std::cin.get();
    return 0;
}
*/

int main() {
    TestEnv env(4455);

    auto ua1 = std::make_shared<UserAgent>(&env);
    auto ua2 = std::make_shared<UserAgent>(&env);

    ua1->Login("lichao");
    ua2->Login("xiaoyun");

    ua1->Call("xiaoyun");

    std::cin.get();

    ua1->Logout();
    ua2->Logout();
}