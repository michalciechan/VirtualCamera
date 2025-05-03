#pragma once

#include <glm/glm.hpp>

#include <array>
#include <vector>
#include <memory>

namespace vcam {

class Entity;

class IComponent {
public:
    virtual ~IComponent() = default;
    virtual void on_update(Entity& entity, float dt) = 0;
};

class Entity {
public:
    Entity() = default;

    Entity(Entity const& other) = delete;
    Entity& operator=(Entity const& other) = delete;

    Entity(Entity&& other) = default;
    Entity& operator=(Entity&& other) = default;

    void on_update(float dt) {
        for (auto& component : m_components) {
            component->on_update(*this, dt);
        }
    }

    IComponent* add_component(std::unique_ptr<IComponent> component) {
        auto* ptr = component.get();
        m_components.push_back(std::move(component));
        return ptr;
    }

    void position(glm::vec3 position) {
        m_position = position;
    }

    glm::vec3& position() {
        return m_position;
    }

    void rotation(glm::vec3 rotation) {
        m_rotation = rotation;
    }

    glm::vec3& rotation() {
        return m_rotation;
    }

    void scale(glm::vec3 scale) {
        m_scale = scale;
    }

    glm::vec3& scale() {
        return m_scale;
    }

private:
    glm::vec3 m_position = glm::vec3(0.0f);
    glm::vec3 m_rotation = glm::vec3(0.0f);
    glm::vec3 m_scale = glm::vec3(1.0f);

    std::vector<std::unique_ptr<IComponent>> m_components;
};

}
