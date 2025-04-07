#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

struct GlobalState {
    bool is_running;
    SDL_Window* const window;
    SDL_Renderer* const renderer;
};

int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Failed to initialize SDL: %s",
            SDL_GetError()
        );
        return 1;
    }

    auto* const window = SDL_CreateWindow(
        "VirtualCamera",
        640,
        480,
        0
    );
    if (window == nullptr) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Failed to create window: %s",
            SDL_GetError()
        );
        SDL_Quit();
        return 1;
    }

    auto* const renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer == nullptr) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Failed to create renderer: %s",
            SDL_GetError()
        );
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    GlobalState state{
        .is_running = true,
        .window = window,
        .renderer = renderer,
    };

    while (state.is_running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_EVENT_QUIT:
                state.is_running = false;
                break;

            default:
                break;
            }
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
