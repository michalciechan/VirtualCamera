#include <vcam/core/entity.hh>
#include <vcam/core/math.hh>
#include <vcam/movement/movement_controller.hh>
#include <vcam/render/model.hh>
#include <vcam/render/camera_component.hh>
#include <vcam/render/render_component.hh>
#include <vcam/render/render_system.hh>
#include <vcam/core/scene.hh>

#include <glm/glm.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cmath>
#include <memory>
#include <random>
#include <string>

struct GlobalState {
    bool is_running;
    vcam::RenderSystem render_system;
    vcam::Scene scene;
    vcam::Entity* camera_entity;
};

const vcam::Model PYRAMID_MODEL(
    vcam::Mesh(
        {
            glm::vec4(0, 0, 0, 1),
            glm::vec4(1, 0, 0, 1),
            glm::vec4(0.5, 0, std::sqrtf(3.0f) / 2, 1),
            glm::vec4(0.5, std::sqrtf(2.0f / 3), std::sqrtf(3.0f) / 6, 1)
        },
        {
            {0, 2, 1},
            {0, 3, 2},
            {1, 2, 3},
            {0, 1, 3}
        },
        {
            {1, 0, 0},
            {0, 1, 0},
            {0, 0, 1},
            {1, 0, 1},
        }
        )
);

void on_init(GlobalState& state);
void on_update(GlobalState& state, float dt);
void on_shutdown(GlobalState& state);

int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION,
            "Failed to initialize SDL: %s",
            SDL_GetError()
        );
        return 1;
    }

    auto* const window = SDL_CreateWindow(
        "VirtualCamera",
        1024,
        768,
        SDL_WINDOW_RESIZABLE
    );
    if (window == nullptr) {
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION,
            "Failed to create window: %s",
            SDL_GetError()
        );
        SDL_Quit();
        return 1;
    }

    auto* const renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer == nullptr) {
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION,
            "Failed to create renderer: %s",
            SDL_GetError()
        );
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    GlobalState state{
        .is_running = true,
        .render_system = vcam::RenderSystem(renderer),
    };

    on_init(state);

    constexpr std::uint32_t MILLISECONDS_PER_SECOND = 1000;
    constexpr std::uint32_t FRAMES_PER_SECOND = 60;

    auto const target = MILLISECONDS_PER_SECOND / FRAMES_PER_SECOND;
    auto last_ticks = SDL_GetTicks();

    while (state.is_running) {
        auto current_ticks = SDL_GetTicks();

        auto const dt = current_ticks - last_ticks;

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

        on_update(state, dt);

        last_ticks = current_ticks;

        current_ticks = SDL_GetTicks();
        auto const elapsed = current_ticks - last_ticks;

        if (elapsed < target) {
            SDL_Delay(target - elapsed);
        }
    }

    on_shutdown(state);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void on_init(GlobalState& state) {
    auto pyramid_model = std::make_shared<vcam::Model>(PYRAMID_MODEL);

    std::mt19937 gen(42);
    std::uniform_real_distribution<float> x(-2.0f, 2.0f);
    std::uniform_real_distribution<float> y(-2.0f, 2.0f);
    std::uniform_real_distribution<float> z(5.0f, 10.0f);
    std::uniform_real_distribution<float> rx(0.0f, 0 * 2 * std::numbers::pi);
    std::uniform_real_distribution<float> ry(0.0f, 0 * 2 * std::numbers::pi);
    std::uniform_real_distribution<float> rz(0.0f, 0 * 2 * std::numbers::pi);
    std::uniform_real_distribution<float> scale(1.0f, 1.0f);
    for (int i = 0; i < 10; ++i) {
        auto pyramid_entity = std::make_shared<vcam::Entity>();
        pyramid_entity->position(glm::vec3(x(gen), y(gen), z(gen)));
        pyramid_entity->rotation(glm::vec3(rx(gen), ry(gen), rz(gen)));
        pyramid_entity->scale(glm::vec3(scale(gen)));
        pyramid_entity->add_component(
            std::make_unique<vcam::RenderComponent>(
                state.render_system,
                pyramid_model
            )
        );
        state.scene.add_entity(pyramid_entity);
    }

    auto camera_entity = std::make_shared<vcam::Entity>();
    camera_entity->add_component(std::make_unique<vcam::MovementController>());
    camera_entity->add_component(std::make_unique<vcam::CameraComponent>(state.render_system));
    state.scene.add_entity(camera_entity);
}

void on_update(GlobalState& state, float dt) {
    for (auto entity : state.scene.entities()) {
        entity->on_update(dt);
    }

    state.render_system.render();
}

void on_shutdown(GlobalState& state) { }
