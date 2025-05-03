#pragma once

#include <vcam/core/entity.hh>

#include <memory>
#include <vector>

namespace vcam {

class Scene {
public:
    void add_entity(std::shared_ptr<Entity> entity) {
        m_entities.push_back(entity);
    }

    std::vector<std::shared_ptr<Entity>> const& entities() const {
        return m_entities;
    }

private:
    std::vector<std::shared_ptr<Entity>> m_entities;
};

}
