#ifndef _RTC_COMMON_TYPES_H_INCLUDED
#define _RTC_COMMON_TYPES_H_INCLUDED

namespace rtc {

class I420VideoFrame {
protected:
    virtual ~I420VideoFrame() = default;
public:
    virtual int width() const = 0;
    virtual int height() const = 0;

    // Returns pointer to the pixel data for a given plane. The memory is owned by
    // the VideoFrameBuffer object and must not be freed by the caller.
    virtual const uint8_t* DataY() const = 0;
    virtual const uint8_t* DataU() const = 0;
    virtual const uint8_t* DataV() const = 0;

    // Returns the number of bytes between successive rows for a given plane.
    virtual int StrideY() const = 0;
    virtual int StrideU() const = 0;
    virtual int StrideV() const = 0;
};

class I420VideoSinkInterface {
public:
    virtual ~I420VideoSinkInterface() = default;
    virtual void OnFrame(const I420VideoFrame& video_frame) = 0;
};
}

#endif // !_RTC_COMMON_TYPES_H_INCLUDED
