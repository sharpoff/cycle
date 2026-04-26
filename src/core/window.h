#pragma once

#include "SDL3/SDL_video.h"
#include "math/math_types.h"
#include <volk.h>

class Window
{
public:
    bool Create(const char *title, uint32_t width, uint32_t height);
    void Destroy();

    bool CreateVKSurface(VkInstance instance, VkSurfaceKHR *surface);

    bool IsMinimized() { return (SDL_GetWindowFlags(m_window) & SDL_WINDOW_MINIMIZED); }

    vec2 GetWindowSize();
    SDL_Window *GetHandle() { return m_window; }

private:
    SDL_Window *m_window;
};