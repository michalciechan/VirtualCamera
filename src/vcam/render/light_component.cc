#include <vcam/core/math.hh>
#include <vcam/render/light_component.hh>

#include <SDL3/SDL_log.h>

namespace vcam {

void LightComponent::on_update(Entity& entity, float dt) {
    static float dir = 1.0f;
    static float speed = 0.001f;

    if (entity.position().x <= -10.0f || entity.position().x >= 10.0f) {
        speed *= -1.0f;
    }

    entity.position().x += speed * dt;

    m_render_system.light(
        Light{
            entity.position(),
            m_ambient_intensity,
            m_specular_intensity,
            m_diffuse_intensity
        });
}

}
