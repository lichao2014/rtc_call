#include "render/interface.h"
#include "render/sdl_renderer.h"

using namespace rtc;

// static 
std::shared_ptr<VideoRendererInterface>
VideoRendererInterface::Create(const char *title, int w, int h) {
    return SDLVideoRenderer::Create(title, w, h);
}

// static 
std::shared_ptr<VideoRendererInterface>
VideoRendererInterface::Create(void *native_handle) {
    return SDLVideoRenderer::Create(native_handle);
}
