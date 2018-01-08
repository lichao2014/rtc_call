#ifndef _SDL_SDL_RENDERER_H_INCLUDED
#define _SDL_SDL_RENDERER_H_INCLUDED

#include "SDL2/SDL.h"

#include "utility/unique_ptr.h"
#include "render/interface.h"

namespace rtc {

class SDLVideoRenderer : public VideoRendererInterface
                       , public std::enable_shared_from_this<SDLVideoRenderer> {
public:
    static std::shared_ptr<SDLVideoRenderer> Create(const char *title, int w, int h);
    static std::shared_ptr<SDLVideoRenderer> Create(void *native_handle);

    bool GetOutputSize(int *w, int *h);
    util::UniquePtr<SDL_Texture> CreateTexture(int w, int h);
    void RenderTexture(const SDL_Rect& rect, SDL_Texture *texture);
private:
    SDLVideoRenderer() = default;
    bool Init(SDL_Window *window);

    std::unique_ptr<I420VideoSinkInterface> CreateSink(float x, float y, float w, float h) override;

    util::UniquePtr<SDL_Window> window_;
    util::UniquePtr<SDL_Renderer> renderer_;
    util::UniquePtr<SDL_mutex> mu_;
};
}
#endif // !_SDL_SDL_RENDERER_H_INCLUDED
