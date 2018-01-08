#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

#include "rtc_base/flags.h"
#include "rtc_base/logging.h"
#include "rtc_base/logsinks.h"
#include "rtc_base/fileutils.h"

#include "rtc_call_interface.h"
#include "render/interface.h"
#include "render/sdl_util.h"

DEFINE_bool(help, false, "print help info");
DEFINE_string(slog, "rtc_session_test.log", "session log file");
DEFINE_string(log, "rtc_log", "call log");
DEFINE_string(stun, "turn:101.132.33.178:3391", "ice server");
DEFINE_string(name, nullptr, "my name");
DEFINE_string(peer, nullptr, "peer name");
DEFINE_string(domain, "101.132.33.178", "login server");
DEFINE_int(sport, 3390, "login port");
DEFINE_int(port, 4455, "login port");


struct CallEnv : rtc::CallEngineOptions {
    std::shared_ptr<rtc::CallEngineInterface> call_engine;
    rtc::CallUserOptions comm_user_options;

    bool Init() {
        call_engine = rtc::CreateCallEngine(*this);
        return (bool)call_engine;
    }

    rtc::CallUserOptions MakeUserOptions(const std::string& name, const std::string& pwd = "123456") {
        auto user_config = comm_user_options;
        user_config.name = name;
        user_config.password = pwd;
        return user_config;
    }

    static std::unique_ptr<CallEnv> CreateDefault() {
        auto env = std::make_unique<CallEnv>();
        env->session.udp_port = FLAG_port;

        rtc::IceServer ice_server;
        ice_server.urls.push_back(FLAG_stun);
        ice_server.username = FLAG_name;
        ice_server.password = "123456";

        env->ice_servers.push_back(ice_server);
        env->session.login_keepalive_sec = 60;

        env->comm_user_options.domain = FLAG_domain;
        env->comm_user_options.login_server_port = FLAG_sport;

        env->Init();

        return env;
    }
};

class CallUser : public rtc::CallUserObserver
               , public rtc::CallObserver {
public:
    explicit CallUser(CallEnv *env, const std::string& name) {
        video_renderer_ = rtc::VideoRendererInterface::Create(name.c_str(), 400, 500);
        user_ = env->call_engine->CreateUser(env->MakeUserOptions(name), this);
    }

    explicit CallUser(CallEnv *env, const std::string& name, const std::string& peer) {
        video_renderer_ = rtc::VideoRendererInterface::Create(name.c_str(), 400, 500);
        peer_ = peer;
        user_ = env->call_engine->CreateUser(env->MakeUserOptions(name), this);
        CheckCall();
    }

private:
    void OnLogin(bool ok) override {
        login_success_ = ok;
        CheckCall();
    }

    void CheckCall() {
        if (login_success_ && user_ && !peer_.empty()) {
            call_ = user_->MakeCall(peer_, this);
        }
    }

    CallObserver *OnCallee(std::shared_ptr<rtc::CallInterface> callee) override {
        call_ = callee;
        return this;
    }

    void OnError() override {

    }

    std::unique_ptr<rtc::I420VideoSinkInterface> OnAddStream(
        bool remote, const std::string&stream_label, const std::string&track_id) {
        if (remote) {
            return video_renderer_->CreateSink(0, 0, 0.8, 0.8);
        } else {
            return video_renderer_->CreateSink(0.8, 0.8, 0.2, 0.2);
        }
    }


    std::shared_ptr<rtc::CallUserInterface> user_;
    std::shared_ptr<rtc::CallInterface> call_;
    std::string peer_;
    bool login_success_ = false;

    std::shared_ptr<rtc::VideoRendererInterface> video_renderer_;
};

int main(int argc, char *argv[]) {
   std::atomic_init

    rtc::FlagList::SetFlagsFromCommandLine(&argc, argv, false);
    if (FLAG_help) {
        rtc::FlagList::Print(nullptr, true);
        return 0;
    }

    auto call_session_log = new rtc::FileRotatingLogSink(FLAG_log, "rtc_call", std::numeric_limits<uint16_t>::max(), 64);
    if (!call_session_log->Init()) {
        return -1;
    }

    rtc::LogMessage::LogThreads();
    rtc::LogMessage::LogTimestamps();
    rtc::LogMessage::AddLogToStream(call_session_log, rtc::LS_INFO);
    RTC_LOG(LS_INFO) << "webrtc log test" << std::endl;

    //rtc_session::SetLogger("file", "STACK", FLAG_slog);


    rtc::SDLInitializer _sdl_initializer;

    auto env = CallEnv::CreateDefault();

    std::unique_ptr<CallUser> user;
    if (FLAG_peer) {
        user.reset(new CallUser(env.get(), FLAG_name, FLAG_peer));
    } else {
        user.reset(new CallUser(env.get(), FLAG_name));
    }
    
    rtc::SDLLoop();

    return 0;
}

