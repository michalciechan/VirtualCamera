#include <vcam/render/camera_component.hh>

#include <SDL3/SDL_keyboard.h>

namespace vcam {

void CameraComponent::on_update(Entity& entity, float dt) {
    auto const* keyboard = SDL_GetKeyboardState(nullptr);

    if (keyboard[SDL_SCANCODE_MINUS]) {
        m_vfov += 0.5f;
    }

    if (keyboard[SDL_SCANCODE_EQUALS]) {
        m_vfov -= 0.5f;
    }

    m_vfov = std::max(std::min(m_vfov, 90.0f), 1.0f);

    Camera camera = {
        .position = entity.position(),
        .rotation = entity.rotation(),
        .vfov = m_vfov
    };
    m_render_system.camera(camera);
}

}
