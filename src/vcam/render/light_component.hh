#pragma once

#include <vcam/core/entity.hh>
#include <vcam/render/model.hh>
#include <vcam/render/render_system.hh>

namespace vcam {

class LightComponent : public IComponent {
public:
    explicit LightComponent(
        RenderSystem& render_system,
        glm::vec3 ambient_intensity,
        glm::vec3 specular_intensity,
        glm::vec3 diffuse_intensity
    ) : m_render_system(render_system),
        m_ambient_intensity{ ambient_intensity },
        m_specular_intensity{ specular_intensity },
        m_diffuse_intensity{ diffuse_intensity } { }

    virtual void on_update(Entity& entity, float dt) override;

private:
    RenderSystem& m_render_system;
    glm::vec3 m_ambient_intensity;
    glm::vec3 m_specular_intensity;
    glm::vec3 m_diffuse_intensity;
};

}
