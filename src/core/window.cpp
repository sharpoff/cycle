#include "window.h"

#include "SDL3/SDL_vulkan.h"
#include "core/logger.h"
#include <SDL3/SDL.h>
#include <stdlib.h>

bool Window::Create(const char *title, uint32_t width, uint32_t height)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        LOGE("Failed to initialize SDL: {}", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    m_window = SDL_CreateWindow(title, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (!m_window) {
        LOGE("Failed to create SDL window: {}", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_RaiseWindow(m_window);
    return true;
}

void Window::Destroy()
{
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

bool Window::CreateVKSurface(VkInstance instance, VkSurfaceKHR *surface)
{
    return SDL_Vulkan_CreateSurface(m_window, instance, nullptr, surface);
}

vec2 Window::GetWindowSize()
{
    uint32_t width = 0, height = 0;
    SDL_GetWindowSize(m_window, (int *)&width, (int *)&height);
    return vec2(width, height);
}