#include "rtc_call_engine.h"

#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"

#include "rtc_call_user.h"

namespace rtc {

std::shared_ptr<CallEngineInterface> 
CreateCallEngine(const CallEngineOptions& options) {
    return CallEngine::Create(options);
}
}

using namespace rtc;

//static 
std::shared_ptr<CallEngine> CallEngine::Create(const CallEngineOptions& options) {
    std::shared_ptr<CallEngine> engine { 
        new CallEngine(options), 
        [](auto p) { delete p; } };
    if (!engine->Initialize()) {
        return nullptr;
    }

    return engine;
}

rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> 
CallEngine::CreatePeerConnectionFactory() {
    return webrtc::CreatePeerConnectionFactory(
        network_thread_.get(),
        worker_thread_.get(),
        signaling_thread_.get(),
        nullptr,
        webrtc::CreateBuiltinAudioEncoderFactory(),
        webrtc::CreateBuiltinAudioDecoderFactory(),
        nullptr,
        nullptr
    );
}

std::unique_ptr<rtc_session::UserInterface>
CallEngine::CreateSessionUser(const rtc_session::UserOptions& options,
                              std::shared_ptr<rtc_session::UserCallback> callback) {
    auto reoptions = options;
    if (options_.session.login_using_sip_rport) {
        reoptions.login_using_sip_rport = options_.session.login_using_sip_rport;
    }

    if (options_.session.login_keepalive_sec) {
        reoptions.login_keepalive_sec.emplace(options_.session.login_keepalive_sec);
    }

    return session_stack_->CreateUser(reoptions, callback);
}

bool CallEngine::Initialize() {
    rtc_session::StackOptions options;
    if (0 != options_.session.udp_port) {
        options.udp_port.emplace(options_.session.udp_port);
    }

    if (0 != options_.session.tcp_port) {
        options.tcp_port.emplace(options_.session.tcp_port);
    }

    options.compression = options_.session.compression;
    session_stack_ = rtc_session::CreateStack(options);
    if (!session_stack_) {
        return false;
    }

    network_thread_ = rtc::Thread::CreateWithSocketServer();
    if (!network_thread_->Start()) {
        return false;
    }

    worker_thread_ = rtc::Thread::Create();
    if (!worker_thread_->Start()) {
        return false;
    }

    signaling_thread_ = rtc::Thread::Create();
    if (!signaling_thread_->Start()) {
        return false;
    }

    return true;
}

std::shared_ptr<CallUserInterface> 
CallEngine::CreateUser(const CallUserOptions& options, CallUserObserver *observer) {
    return CallUser::Create(options, observer, shared_from_this());
}
