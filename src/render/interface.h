#ifndef _RTC_VIDEO_RENDER_INTERFACE_H_INCLUDED
#define _RTC_VIDEO_RENDER_INTERFACE_H_INCLUDED

#include <memory>

#include "rtc_common_types.h"

namespace rtc {

class VideoRendererInterface {
protected:
    virtual ~VideoRendererInterface() = default;
public:
    static std::shared_ptr<VideoRendererInterface> Create(const char *title, int w, int h);
    static std::shared_ptr<VideoRendererInterface> Create(void *native_handle);

    virtual std::unique_ptr<I420VideoSinkInterface> CreateSink(float x, float y, float w, float h) = 0;
};
}

#endif // !_RTC_VIDEO_RENDER_INTERFACE_H_INCLUDED
