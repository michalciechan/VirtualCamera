#pragma once

#include <vcam/core/entity.hh>

namespace vcam {

class MovementController : public IComponent {
public:
    virtual void on_update(Entity& entity, float dt) override;

private:
    glm::vec3 calculate_rotation_delta(bool const* keyboard, float dt);
    glm::vec3 calculate_translation_delta(bool const* keyboard, float dt);
};

}
