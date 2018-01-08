#include "render/interface.h"
#include "render/sdl_util.h"
#include "rtc_video_sink.h"
#include "rtc_video_capturer.h"


int main() {
    rtc::SDLInitializer _sdl_initializer;

    auto renderer = rtc::VideoRendererInterface::Create("123", 400, 500);
    auto sink1 = std::make_unique<rtc::VideoSinkAdapter>(renderer->CreateSink(0, 0, 0.5, 0.5));
    auto sink2 = std::make_unique<rtc::VideoSinkAdapter>(renderer->CreateSink(0.5, 0.5, 0.5, 0.5));

    auto capturer1 = rtc::OpenVideoCaptureDevice();
    capturer1->AddOrUpdateSink(sink1.get(), {});
    capturer1->StartCapturing(capturer1->GetSupportedFormats()->at(0));

    auto capturer2 = rtc::OpenVideoCaptureDevice();
    capturer2->AddOrUpdateSink(sink2.get(), {});
    capturer2->StartCapturing(capturer2->GetSupportedFormats()->at(1));

    rtc::SDLLoop();

}