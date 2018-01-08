#ifndef _RTC_CALL_SINK_VIDEO_H_INCLUDED
#define _RTC_CALL_SINK_VIDEO_H_INCLUDED


#include "api/mediastreaminterface.h"

#include "rtc_common_types.h"

namespace rtc {

class VideoFrameAdapter : public I420VideoFrame {
public:
    VideoFrameAdapter(const webrtc::VideoFrame& webrtc_frame)
        : webrtc_frame_(webrtc_frame)
        , i420_buffer_(webrtc_frame.video_frame_buffer()->ToI420()) {}

private:
    int width() const override {
        return webrtc_frame_.width();
    }

    int height() const override {
        return webrtc_frame_.height();
    }

    const uint8_t* DataY() const override {
        return i420_buffer_->DataY();
    }

    const uint8_t* DataU() const  override {
        return i420_buffer_->DataU();
    }

    const uint8_t* DataV() const  override {
        return i420_buffer_->DataV();
    }

    int StrideY() const  override {
        return i420_buffer_->StrideY();
    }

    int StrideU() const override {
        return i420_buffer_->StrideU();
    }

    int StrideV() const  override {
        return i420_buffer_->StrideV();
    }

    const webrtc::VideoFrame& webrtc_frame_;
    rtc::scoped_refptr<webrtc::I420BufferInterface> i420_buffer_;
};

class VideoSinkAdapter : public rtc::VideoSinkInterface<webrtc::VideoFrame> {
public:
    VideoSinkAdapter(std::unique_ptr<I420VideoSinkInterface> i420_video_sink)
        : i420_video_sink_(std::move(i420_video_sink)) {}
private:
    void OnFrame(const webrtc::VideoFrame& frame) override {
        VideoFrameAdapter i420_video_frame(frame);
        i420_video_sink_->OnFrame(i420_video_frame);
    }

    std::unique_ptr<I420VideoSinkInterface> i420_video_sink_;
};
}

#endif // !_RTC_CALL_SINK_VIDEO_H_INCLUDED
