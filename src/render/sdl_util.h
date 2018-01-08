#ifndef _RTC_SDL_UTIL_H_INCLUDED
#define _RTC_SDL_UTIL_H_INCLUDED

#include "SDL2/SDL.h"
#undef main

namespace rtc {

struct SDLInitializer {
    SDLInitializer() {
        ::SDL_Init(SDL_INIT_EVERYTHING);
    }

    ~SDLInitializer() {
        ::SDL_Quit();
    }
};

inline void SDLLoop() {
    SDL_Event event;
    while (::SDL_WaitEvent(&event), SDL_QUIT != event.type);
}
}

#endif // !_RTC_SDL_UTIL_H_INCLUDED

