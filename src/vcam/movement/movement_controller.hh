#pragma once

#include <vcam/core/entity.hh>

namespace vcam {

class MovementController : public IComponent {
public:
    virtual void on_update(Entity& entity, float dt) override;
};

}
