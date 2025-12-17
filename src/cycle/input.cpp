#include "cycle/input.h"
#include "SDL3/SDL_events.h"
#include "cycle/engine.h"

namespace Input
{
    void Init()
    {
    }

    void Shutdown()
    {
    }

    void Process()
    {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED || event.key.key == SDLK_ESCAPE && event.key.down) {
                Engine::running = false;
            }
        }
    }
} // namespace Input