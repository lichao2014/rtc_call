#include "rtc_call.h"

#include "json/json.h"
#include "json/writer.h"
#include "json/reader.h"

#include "utility/scoped_guard.h"
#include "rtc_call_user.h"
#include "rtc_video_capturer.h"

using namespace rtc;

namespace {

std::string empty_string;
std::string json_mime_type("application/json");

rtc_session::Contents MakeSdpContents(const Json::Value& body) {
    Json::FastWriter w;
    return { json_mime_type, w.write(body) };
}
}

//static 
std::shared_ptr<Call> Call::CreateCaller(CallUser& user,
    const rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>& pc_factory,
    std::unique_ptr<rtc_session::CallerInterface> caller,
    CallObserver *observer) {
    std::shared_ptr<Call> call(new Call(user, pc_factory));
    if (!call->InitCaller(std::move(caller), observer)) {
        return nullptr;
    }
    return call;
}

//static 
std::shared_ptr<Call> Call::CreateCallee(CallUser& user,
    const rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>& pc_factory,
    std::unique_ptr<rtc_session::CalleeInterface> callee) {
    std::shared_ptr<Call> call(new Call(user, pc_factory));
    if (!call->InitCallee(std::move(callee))) {
        return nullptr;
    }
    return call;
}

bool Call::InitCaller(std::unique_ptr<rtc_session::CallerInterface> caller,
                      CallObserver *observer) {
    SetObserver(observer);

    if (!CreatePeerConnectionAndStreams()) {
        return false;
    }

    caller->SetCallback(shared_from_this());
    caller_ = std::move(caller);

    pc_->CreateOffer(CreateSessionDescriptionObserver::Create(this), nullptr);
    return true;
}

bool Call::InitCallee(std::unique_ptr<rtc_session::CalleeInterface> callee) {
    callee->SetCallback(shared_from_this());
    callee_ = std::move(callee);
    return true;
}

bool Call::CreatePeerConnectionAndStreams() {
    
    webrtc::PeerConnectionInterface::RTCConfiguration config;
    for (auto& ice_server : user_.engine()->options().ice_servers) {
        webrtc::PeerConnectionInterface::IceServer server;
        server.urls = ice_server.urls;
        server.username = ice_server.username;
        server.password = ice_server.password;

        config.servers.push_back(server);
    }

    pc_ = pc_factory_->CreatePeerConnection(config, nullptr, nullptr, this);

    auto stream = pc_factory_->CreateLocalMediaStream("stream1");
    auto capture_device = OpenVideoCaptureDevice();
    if (!capture_device) {
        auto audio_track = pc_factory_->CreateAudioTrack("audio",
            pc_factory_->CreateAudioSource(nullptr));
        if (!stream->AddTrack(audio_track)) {
            return false;
        }
    } else {
        auto video_track = pc_factory_->CreateVideoTrack("video",
            pc_factory_->CreateVideoSource(std::move(capture_device)));

        if (!stream->AddTrack(video_track)) {
            return false;
        }

        AddStream(stream->label(), video_track);
    }

    if (!pc_->AddStream(stream)) {
        return false;
    }

    return true;
}

rtc_session::CallInterface *Call::call() {
    if (caller_) {
        return caller_.get();
    }

    if (callee_) {
        return callee_.get();
    }

    return nullptr;
}

void Call::AddStream(const std::string& stream_label,
                     rtc::scoped_refptr<webrtc::VideoTrackInterface> track) {
    auto sink = std::make_unique<VideoSinkAdapter>(
        observer_->OnAddStream(
        track->GetSource()->remote(),
        stream_label,
        track->id()));

    track->AddOrUpdateSink(sink.get(), {});
    sinks_.insert({ track.get(), std::move(sink) });
}

const CallUserInterface *Call::user() const {
    return &user_;
}

const std::string& Call::peer() const {
    if (caller_) {
        return caller_->peer().name;
    }

    if (callee_) {
        return callee_->peer().name;
    }

    return empty_string;
}

void Call::OnInit() {

}

void Call::OnFailure() {

}

void Call::OnOffer(const std::string& offer) {
    auto reject_response = util::MakeScopedGuard([&] {
        callee_->Reject();
    });

    if (!CreatePeerConnectionAndStreams()) {
        return;
    }

    webrtc::SdpParseError error;
    auto desc = webrtc::CreateSessionDescription(
        webrtc::SessionDescriptionInterface::kOffer, 
        offer, 
        &error);
    if (!desc) {
        return;
    }

    pc_->SetRemoteDescription(SetSessionDescriptionObserver::Create(this, true), desc);
    pc_->CreateAnswer(CreateSessionDescriptionObserver::Create(this), nullptr);

    reject_response.depose();
}

void Call::OnAnswer(const std::string& answer) {
    std::string offer;
    if (!caller_->GetLocalSdp(&offer)) {
        return;
    }

    webrtc::SdpParseError error;
    auto local_desc = webrtc::CreateSessionDescription(
        webrtc::SessionDescriptionInterface::kOffer, 
        offer, 
        &error);
    if (!local_desc) {
        return;
    }
    pc_->SetLocalDescription(
        SetSessionDescriptionObserver::Create(this, false), local_desc);

    auto remote_desc = webrtc::CreateSessionDescription(
        webrtc::SessionDescriptionInterface::kAnswer, 
        answer, 
        &error);
    if (!local_desc) {
        return;
    }
    pc_->SetRemoteDescription(
        SetSessionDescriptionObserver::Create(this, true), 
        remote_desc);
}

void Call::OnMessage(const rtc_session::Contents& msg) {
    auto reject_response = util::MakeScopedGuard([&] {
        call()->RejectNIT();
    });

    if (json_mime_type != msg.type) {
        return;
    }

    Json::Reader reader;
    Json::Value body;

    if (!reader.parse(msg.body, body)) {
        return;
    }

    if (!body.isObject()) {
        return;
    }

    if (!body.isMember("sdp_mid") || !body["sdp_mid"].isString()) {
        return;
    }

    if (!body.isMember("sdp_mline_index") || !body["sdp_mline_index"].isInt()) {
        return;
    }

    if (!body.isMember("sdp") || !body["sdp"].isString()) {
        return;
    }

    const auto& sdp_mid = body["sdp_mid"].asString();
    auto sdp_mline_index = body["sdp_mline_index"].asInt();
    const auto& sdp = body["sdp"].asString();

    webrtc::SdpParseError error;
    std::unique_ptr<webrtc::IceCandidateInterface> candidate{
        webrtc::CreateIceCandidate(sdp_mid, sdp_mline_index, sdp, &error) };
    if (!candidate) {
        return;
    }

    if (!pc_->AddIceCandidate(candidate.get())) {
        return;
    }

    reject_response.depose();
    call()->AcceptNIT();
}

void Call::OnMessageResult(bool success) {

}

void Call::OnConnected() {

}

void Call::OnTerminated() {
    if (callee_) {
        user_.OnCalleeQuit(this);
    }
}

void Call::OnSignalingChange(
    webrtc::PeerConnectionInterface::SignalingState new_state)  {

}

void Call::OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {
    auto video_tracks = stream->GetVideoTracks();
    for (auto&& video_track : video_tracks) {
        AddStream(stream->label(), video_track);
    }
}

void Call::OnRemoveStream(
    rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {
    auto video_tracks = stream->GetVideoTracks();
    for (auto&& video_track : video_tracks) {
        video_track->RemoveSink(sinks_[video_track].get());
    }
}

void Call::OnDataChannel(
    rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) {

}

void Call::OnRenegotiationNeeded() {

}

void Call::OnIceConnectionChange(
    webrtc::PeerConnectionInterface::IceConnectionState new_state) {
}

void Call::OnIceGatheringChange(
    webrtc::PeerConnectionInterface::IceGatheringState new_state) {
}

void Call::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
    std::string sdp;
    candidate->ToString(&sdp);

    Json::Value body;
    body["sdp_mid"] = candidate->sdp_mid();
    body["sdp_mline_index"] = candidate->sdp_mline_index();
    body["sdp"] = sdp;

    auto json_contents = MakeSdpContents(body);
    if (caller_) {
        caller_->Message(json_contents);
    }

    if (callee_) {
        callee_->Message(json_contents);\
    }
}

void Call::OnIceCandidatesRemoved(const std::vector<cricket::Candidate>& candidates) {

}

void Call::OnIceConnectionReceivingChange(bool receiving) {
}

void Call::OnAddTrack(
    rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
    const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& streams) {
}

void Call::OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) {
}

void Call::OnCreateSessionDescriptionSuccess(
    webrtc::SessionDescriptionInterface* desc) {
    std::string sdp;
    desc->ToString(&sdp);

    if (caller_) {
        caller_->Invite(&sdp);
    } 

    if (callee_) {
        pc_->SetLocalDescription(
            SetSessionDescriptionObserver::Create(this, false), 
            desc);
        callee_->Accept(sdp);
    }
}

void Call::OnCreateSessionDescriptionFailure(const std::string& error) {

}

void Call::OnSetSessionDescriptionSuccess(bool remote) {

}

void Call::OnSetSessionDescriptionFailure(bool remote, const std::string& e) {

}
