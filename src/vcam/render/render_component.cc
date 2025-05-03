#include <vcam/core/math.hh>
#include <vcam/render/render_component.hh>

namespace vcam {

void RenderComponent::on_update(Entity& entity, float dt) {
    auto const model_to_scene_transform = calculate_transform_matrix(
        entity.position(),
        entity.rotation(),
        entity.scale()
    );
    m_render_system.add_instance(*m_model.get(), model_to_scene_transform);
}

}
