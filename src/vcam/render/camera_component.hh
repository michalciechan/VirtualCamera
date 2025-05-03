#pragma once

#include <vcam/core/entity.hh>
#include <vcam/render/render_system.hh>

namespace vcam {

class CameraComponent : public IComponent {
public:
    CameraComponent(RenderSystem& render_system) : m_render_system{ render_system } { }

    virtual void on_update(Entity& entity, float dt) override;

private:
    RenderSystem& m_render_system;
    float m_vfov = 30.0f;
};

}
