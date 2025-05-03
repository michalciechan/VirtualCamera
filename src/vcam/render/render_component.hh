#pragma once

#include <vcam/core/entity.hh>
#include <vcam/render/model.hh>
#include <vcam/render/render_system.hh>

namespace vcam {

class RenderComponent : public IComponent {
public:
    explicit RenderComponent(RenderSystem& render_system, std::shared_ptr<Model> model)
        : m_render_system(render_system), m_model(model) { }

    virtual void on_update(Entity& entity, float dt) override;

private:
    RenderSystem& m_render_system;
    std::shared_ptr<Model> m_model;
};

}
