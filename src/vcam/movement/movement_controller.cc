#include <vcam/core/math.hh>
#include <vcam/movement/movement_controller.hh>

#include <SDL3/SDL_keyboard.h>

namespace vcam {

static glm::vec3 calculate_rotation_delta(bool const* keyboard, float dt);
static glm::vec3 calculate_translation_delta(bool const* keyboard, float dt);

void MovementController::on_update(Entity& entity, float dt) {
    auto* keyboard = SDL_GetKeyboardState(nullptr);

    auto const dr = calculate_rotation_delta(keyboard, dt);
    auto const dtr = calculate_translation_delta(keyboard, dt);

    auto rotation = entity.rotation();
    auto position = entity.position();

    auto const local_delta_transform = calculate_transform_matrix(dtr, dr, glm::vec3(1.0f));
    auto const local_to_scene_transform = calculate_transform_matrix(position, rotation, glm::vec3(1.0f));
    auto const new_local_to_scene_transform = local_to_scene_transform * local_delta_transform;

    auto const new_position = glm::vec3(new_local_to_scene_transform * glm::vec4(glm::vec3(0.0f), 1.0f));

    glm::vec3 new_rotation;
    glm::extractEulerAngleYXZ(
        new_local_to_scene_transform,
        new_rotation.y,
        new_rotation.x,
        new_rotation.z
    );

    entity.position(new_position);
    entity.rotation(new_rotation);
}

glm::vec3 calculate_rotation_delta(bool const* keyboard, float dt) {
    auto speed = 0.0005f;

    glm::vec3 rotation(0.0f);

    if (keyboard[SDL_SCANCODE_UP]) {
        rotation.x -= speed * dt;
    }

    if (keyboard[SDL_SCANCODE_DOWN]) {
        rotation.x += speed * dt;
    }

    if (keyboard[SDL_SCANCODE_LEFT]) {
        rotation.y -= speed * dt;
    }

    if (keyboard[SDL_SCANCODE_RIGHT]) {
        rotation.y += speed * dt;
    }

    if (keyboard[SDL_SCANCODE_Q]) {
        rotation.z += speed * dt;
    }

    if (keyboard[SDL_SCANCODE_E]) {
        rotation.z -= speed * dt;
    }

    return rotation;
}

glm::vec3 calculate_translation_delta(bool const* keyboard, float dt) {
    auto speed = 0.0025f;
    if (keyboard[SDL_SCANCODE_LSHIFT]) {
        speed *= 2.0f;
    }

    glm::vec3 translation(0.0f);

    if (keyboard[SDL_SCANCODE_W]) {
        translation.z += speed * dt;
    }

    if (keyboard[SDL_SCANCODE_S]) {
        translation.z -= speed * dt;
    }

    if (keyboard[SDL_SCANCODE_A]) {
        translation.x -= speed * dt;
    }

    if (keyboard[SDL_SCANCODE_D]) {
        translation.x += speed * dt;
    }

    if (keyboard[SDL_SCANCODE_LCTRL]) {
        translation.y -= speed * dt;
    }

    if (keyboard[SDL_SCANCODE_SPACE]) {
        translation.y += speed * dt;
    }

    return translation;
}

}
