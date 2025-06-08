#pragma once

#include <glm/glm.hpp>

#include <array>
#include <vector>

namespace vcam {

class Mesh {
public:
    explicit Mesh(
        std::vector<glm::vec3> vertices,
        std::vector<std::array<std::size_t, 3>> triangles,
        std::vector<std::array<glm::vec3, 3>> triangle_normals
    )
        : m_vertices{ vertices }, m_triangles{ triangles }, m_triangle_normals{ triangle_normals } { }

    std::vector<glm::vec3> const& vertices() const {
        return m_vertices;
    }

    std::vector<std::array<std::size_t, 3>> const& triangles() const {
        return m_triangles;
    }

    std::vector<std::array<glm::vec3, 3>> const& triangle_normals() const {
        return m_triangle_normals;
    }

private:
    std::vector<glm::vec3> m_vertices;
    std::vector<std::array<std::size_t, 3>> m_triangles;
    std::vector<std::array<glm::vec3, 3>> m_triangle_normals;
};

class Material {
public:
    explicit Material(
        glm::vec3 color,
        float shininess,
        glm::vec3 specular_reflection,
        glm::vec3 diffuse_reflection,
        glm::vec3 ambient_reflection
    ) : m_color{ color },
        m_shininess{ shininess },
        m_specular_reflection{ specular_reflection },
        m_diffuse_reflection{ diffuse_reflection },
        m_ambient_reflection{ ambient_reflection } { }

    glm::vec3 const& color() const {
        return m_color;
    }

    float shininess() const {
        return m_shininess;
    }

    glm::vec3 specular_reflection() const {
        return m_specular_reflection;
    }

    glm::vec3 diffuse_reflection() const {
        return m_diffuse_reflection;
    }

    glm::vec3 ambient_reflection() const {
        return m_ambient_reflection;
    }

private:
    glm::vec3 m_color;
    float m_shininess;
    glm::vec3 m_specular_reflection;
    glm::vec3 m_diffuse_reflection;
    glm::vec3 m_ambient_reflection;
};

class Model {
public:
    explicit Model(Mesh mesh, Material material)
        : m_mesh{ mesh }, m_material{ material } { }

    Mesh const& mesh() const {
        return m_mesh;
    }

    Material const& material() const {
        return m_material;
    }

private:
    Mesh m_mesh;
    Material  m_material;
};

}
