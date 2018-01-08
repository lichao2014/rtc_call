#include "render/sdl_renderer.h"

using namespace rtc;

namespace {

class SDLVideoSink : public I420VideoSinkInterface {
public:
    SDLVideoSink(std::shared_ptr<SDLVideoRenderer> renderer, float x, float y, float w, float h)
        : renderer_(renderer)
        , x_(x)
        , y_(y)
        , w_(w)
        , h_(h) {}

private:
    void OnFrame(const I420VideoFrame& video_frame) override {
        UpdateYUVTexture(video_frame);

        SDL_Rect rect;
        if (CalcTextureRect(&rect)) {
            renderer_->RenderTexture(rect, texture_.get());
        }
    }

    void UpdateYUVTexture(const I420VideoFrame& frame) {
        if (texture_) {
            int w = 0;
            int h = 0;

            ::SDL_QueryTexture(texture_.get(), nullptr, nullptr, &w, &h);
            if (frame.width() != w || frame.height() != h) {
                texture_.reset();
            }
        }

        if (!texture_) {
            texture_ = renderer_->CreateTexture(frame.width(), frame.height());
        }

        ::SDL_UpdateYUVTexture(
            texture_.get(),
            nullptr,
            frame.DataY(),
            frame.StrideY(),
            frame.DataU(),
            frame.StrideU(),
            frame.DataV(),
            frame.StrideV()
        );
    }

    bool CalcTextureRect(SDL_Rect *rect) {
        int w;
        int h;
        if (!renderer_->GetOutputSize(&w, &h)) {
            return false;
        }

        rect->x = static_cast<int>(x_ * w);
        rect->y = static_cast<int>(y_ * h);
        rect->w = static_cast<int>(w_ * w);
        rect->h = static_cast<int>(h_ * h);

        if (rect->w > w) {
            rect->w = w;
        }

        if (rect->h > h) {
            rect->h = h;
        }

        return true;
    }

    std::shared_ptr<SDLVideoRenderer> renderer_;
    util::UniquePtr<SDL_Texture> texture_;

    float x_;
    float y_;
    float w_;
    float h_;
};
}

// static
std::shared_ptr<SDLVideoRenderer>
SDLVideoRenderer::Create(const char *title, int w, int h) {
    auto window = ::SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        w,
        h,
        SDL_WINDOW_RESIZABLE);

    if (!window) {
        return nullptr;
    }

    std::shared_ptr<SDLVideoRenderer> render(new SDLVideoRenderer);
    if (!render->Init(std::move(window))) {
        return nullptr;
    }

    return render;
}

//static 
std::shared_ptr<SDLVideoRenderer>
SDLVideoRenderer::Create(void *native_handle) {
    auto window = ::SDL_CreateWindowFrom(native_handle);
    if (!window) {
        return nullptr;
    }

    std::shared_ptr<SDLVideoRenderer> render(new SDLVideoRenderer);
    if (!render->Init(std::move(window))) {
        return nullptr;
    }

    return render;
}

bool SDLVideoRenderer::GetOutputSize(int *w, int *h) {
    return ::SDL_GetRendererOutputSize(renderer_.get(), w, h) >= 0;
}

util::UniquePtr<SDL_Texture> SDLVideoRenderer::CreateTexture(int w, int h) {
    return {
        ::SDL_CreateTexture(
            renderer_.get(),
            SDL_PIXELFORMAT_IYUV,
            SDL_TEXTUREACCESS_STREAMING,
            w,
            h),
        ::SDL_DestroyTexture
    };
}

void SDLVideoRenderer::RenderTexture(const SDL_Rect& rect, SDL_Texture *texture) {
    //::SDL_RenderClear(renderer_.get());
    ::SDL_LockMutex(mu_.get());

    ::SDL_RenderCopy(renderer_.get(), texture, nullptr, &rect);
    ::SDL_RenderPresent(renderer_.get());

    ::SDL_UnlockMutex(mu_.get());
    //SDL_UpdateWindowSurfaceRects(window_.get(), &rect, 1);
}

bool SDLVideoRenderer::Init(SDL_Window *window) {
    util::UniquePtr<SDL_Renderer> renderer{
        SDL_CreateRenderer(
            window,
            -1,
            SDL_RENDERER_TARGETTEXTURE),
        ::SDL_DestroyRenderer
    };

    if (!renderer) {
        return false;
    }

    window_ = util::UniquePtr<SDL_Window>(window, ::SDL_DestroyWindow);
    renderer_ = std::move(renderer);
    mu_ = util::UniquePtr<SDL_mutex>{ ::SDL_CreateMutex(), ::SDL_DestroyMutex };

    return true;
}

std::unique_ptr<I420VideoSinkInterface>
SDLVideoRenderer::CreateSink(float x, float y, float w, float h) {
    return std::make_unique<SDLVideoSink>(shared_from_this(), x, y, w, h);
}
