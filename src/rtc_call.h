#ifndef _RTC_CALL_H_INCLUDED
#define _RTC_CALL_H_INCLUDED

#include <map>

#include "api/peerconnectioninterface.h"

#include "rtc_call_interface.h"
#include "rtc_video_sink.h"
#include "session/interface.h"

namespace rtc {

class CallUser;

class Call : public CallInterface
           , public rtc_session::CallCallback
           , public webrtc::PeerConnectionObserver
           , public std::enable_shared_from_this<Call> {
public:
    static std::shared_ptr<Call> CreateCaller(CallUser& user,
        const rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>& pc_factory,
        std::unique_ptr<rtc_session::CallerInterface> caller,
        CallObserver *observer);

    static std::shared_ptr<Call> CreateCallee(CallUser& user,
        const rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>& pc_factory,
        std::unique_ptr<rtc_session::CalleeInterface> callee);

    void SetObserver(CallObserver *observer) { observer_ = observer; }
private:
    Call(CallUser& user,
        const rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>& pc_factory)
        : user_(user)
        , pc_factory_(pc_factory) {}

    bool InitCaller(std::unique_ptr<rtc_session::CallerInterface> caller, CallObserver *observer);
    bool InitCallee(std::unique_ptr<rtc_session::CalleeInterface> callee);
    bool CreatePeerConnectionAndStreams();
    rtc_session::CallInterface *call();
    void AddStream(const std::string& stream_label, rtc::scoped_refptr<webrtc::VideoTrackInterface> track);

    const CallUserInterface *user() const override;
    const std::string& peer() const override;

    void OnInit() override;
    void OnFailure() override;
    void OnOffer(const std::string& offer) override;
    void OnAnswer(const std::string& answer) override;
    void OnMessage(const rtc_session::Contents& msg) override;
    void OnMessageResult(bool success) override;
    void OnConnected() override;
    void OnTerminated() override;

    // Triggered when the SignalingState changed.
    void OnSignalingChange(
        webrtc::PeerConnectionInterface::SignalingState new_state) override;

    // TODO(deadbeef): Once all subclasses override the scoped_refptr versions
    // of the below three methods, make them pure virtual and remove the raw
    // pointer version.

    // Triggered when media is received on a new stream from remote peer.
    void OnAddStream(
        rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;

    // Triggered when a remote peer close a stream.
    void OnRemoveStream(
        rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;

    // Triggered when a remote peer opens a data channel.
    void OnDataChannel(
        rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override;
    // Triggered when renegotiation is needed. For example, an ICE restart
    // has begun.
    void OnRenegotiationNeeded() override;

    // Called any time the IceConnectionState changes.
    //
    // Note that our ICE states lag behind the standard slightly. The most
    // notable differences include the fact that "failed" occurs after 15
    // seconds, not 30, and this actually represents a combination ICE + DTLS
    // state, so it may be "failed" if DTLS fails while ICE succeeds.
    void OnIceConnectionChange(
        webrtc::PeerConnectionInterface::IceConnectionState new_state) override;

    // Called any time the IceGatheringState changes.
    void OnIceGatheringChange(
        webrtc::PeerConnectionInterface::IceGatheringState new_state) override;

    // A new ICE candidate has been gathered.
    void OnIceCandidate(
        const webrtc::IceCandidateInterface* candidate) override;

    // Ice candidates have been removed.
    // TODO(honghaiz): Make this a pure virtual method when all its subclasses
    // implement it.
    void OnIceCandidatesRemoved(
        const std::vector<cricket::Candidate>& candidates) override;

    // Called when the ICE connection receiving status changes.
    void OnIceConnectionReceivingChange(bool receiving) override;

    // This is called when a receiver and its track is created.
    // TODO(zhihuang): Make this pure virtual when all subclasses implement it.
    void OnAddTrack(
        rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
        const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& streams) override;

    // TODO(hbos,deadbeef): Add |OnAssociatedStreamsUpdated| with |receiver| and
    // |streams| as arguments. This should be called when an existing receiver its
    // associated streams updated. https://crbug.com/webrtc/8315
    // This may be blocked on supporting multiple streams per sender or else
    // this may count as the removal and addition of a track?
    // https://crbug.com/webrtc/7932

    // Called when a receiver is completely removed. This is current (Plan B SDP)
    // behavior that occurs when processing the removal of a remote track, and is
    // called when the receiver is removed and the track is muted. When Unified
    // Plan SDP is supported, transceivers can change direction (and receivers
    // stopped) but receivers are never removed.
    // https://w3c.github.io/webrtc-pc/#process-remote-track-removal
    // TODO(hbos,deadbeef): When Unified Plan SDP is supported and receivers are
    // no longer removed, deprecate and remove this callback.
    // TODO(hbos,deadbeef): Make pure virtual when all subclasses implement it.
    void OnRemoveTrack(
        rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;

    void OnCreateSessionDescriptionSuccess(
        webrtc::SessionDescriptionInterface* desc);

    void OnCreateSessionDescriptionFailure(const std::string& error);

    void OnSetSessionDescriptionSuccess(bool remote);
    void OnSetSessionDescriptionFailure(bool remote, const std::string& e);

    class CreateSessionDescriptionObserver 
        : public webrtc::CreateSessionDescriptionObserver {
    public:
        static CreateSessionDescriptionObserver *Create(Call *call) {
            return new rtc::RefCountedObject<CreateSessionDescriptionObserver>{ call };
        }

        explicit CreateSessionDescriptionObserver(Call *call) : call_(call) {}
    private:
        void OnSuccess(webrtc::SessionDescriptionInterface* desc) override {
            call_->OnCreateSessionDescriptionSuccess(desc);
        }

        void OnFailure(const std::string& error) override {
            call_->OnCreateSessionDescriptionFailure(error);
        }

        Call *call_;
    };

    class SetSessionDescriptionObserver 
        : public webrtc::SetSessionDescriptionObserver {
    public:
        static SetSessionDescriptionObserver *Create(Call *call, bool remote) {
            return new rtc::RefCountedObject<SetSessionDescriptionObserver>{ call, remote };
        }

        SetSessionDescriptionObserver(Call *call, bool remote = false)
            : call_(call)
            , remote_(remote) {
        }
    private:
        void OnSuccess() override {
            call_->OnSetSessionDescriptionSuccess(remote_);
        }

        void OnFailure(const std::string& e) override {
            call_->OnSetSessionDescriptionFailure(remote_, e);
        }

        Call *call_;
        bool remote_;
    };

    CallUser &user_;
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> pc_factory_;

    std::unique_ptr<rtc_session::CallerInterface> caller_;
    std::unique_ptr<rtc_session::CalleeInterface> callee_;

    rtc::scoped_refptr<webrtc::PeerConnectionInterface> pc_;

    CallObserver *observer_ = nullptr;

    std::map<webrtc::VideoTrackInterface *, std::unique_ptr<VideoSinkAdapter>> sinks_;
};
}

#endif // _RTC_CALL_H_INCLUDED
