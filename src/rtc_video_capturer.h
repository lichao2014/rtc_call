#ifndef _RTC_VIDEO_CAPTURER_H_INCLUDED
#define _RTC_VIDEO_CAPTURER_H_INCLUDED

#include "media/base/videocapturer.h"
#include "modules/video_capture/video_capture_factory.h"
#include "media/engine/webrtcvideocapturerfactory.h"

namespace rtc {

std::unique_ptr<cricket::VideoCapturer> OpenVideoCaptureDevice();
}

#endif // !_RTC_VIDEO_CAPTURER_H_INCLUDED
