#pragma once

#include <glm/glm.hpp>

#include <array>
#include <vector>

namespace vcam {

class Mesh {
public:
    explicit Mesh(
        std::vector<glm::vec4> vertices,
        std::vector<std::array<std::size_t, 3>> triangles,
        std::vector<glm::vec3> colors
    )
        : m_vertices{ vertices }, m_triangles{ triangles }, m_colors{ colors } { }

    std::vector<glm::vec4> const& vertices() const {
        return m_vertices;
    }

    std::vector<std::array<std::size_t, 3>> const& triangles() const {
        return m_triangles;
    }

    std::vector<glm::vec3> const& colors() const {
        return m_colors;
    }

private:
    std::vector<glm::vec4> m_vertices;
    std::vector<std::array<std::size_t, 3>> m_triangles;
    std::vector<glm::vec3> m_colors;
};

class Model {
public:
    explicit Model(Mesh mesh)
        : m_mesh{ mesh } { }

    Mesh const& mesh() const {
        return m_mesh;
    }

private:
    Mesh m_mesh;
};

}
