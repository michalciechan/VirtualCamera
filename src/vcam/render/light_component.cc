#include <vcam/core/math.hh>
#include <vcam/render/light_component.hh>

#include <SDL3/SDL_log.h>

namespace vcam {

static glm::vec3 calculate_translation_delta(bool const* keyboard, float dt);

void LightComponent::on_update(Entity& entity, float dt) {
    auto* const keyboard = SDL_GetKeyboardState(nullptr);

    auto rotation = entity.rotation();
    auto position = entity.position();

    auto const dtr = calculate_translation_delta(keyboard, dt);
    auto const local_delta_transform = calculate_transform_matrix(dtr, glm::vec3(0.0f), glm::vec3(1.0f));
    auto const local_to_scene_transform = calculate_transform_matrix(position, rotation, glm::vec3(1.0f));
    auto const new_local_to_scene_transform = local_to_scene_transform * local_delta_transform;

    auto const new_position = glm::vec3(new_local_to_scene_transform * glm::vec4(glm::vec3(0.0f), 1.0f));

    entity.position(new_position);

    m_render_system.light(
        Light{
            entity.position(),
            m_ambient_intensity,
            m_specular_intensity,
            m_diffuse_intensity
        });
}

glm::vec3 calculate_translation_delta(bool const* keyboard, float dt) {
    auto speed = 0.0025f;
    if (keyboard[SDL_SCANCODE_LSHIFT]) {
        speed *= 2.0f;
    }

    glm::vec3 translation(0.0f);

    if (keyboard[SDL_SCANCODE_KP_4]) {
        translation.x -= speed * dt;
    }

    if (keyboard[SDL_SCANCODE_KP_6]) {
        translation.x += speed * dt;
    }

    return translation;
}

}
