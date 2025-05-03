#pragma once

#include <glm/glm.hpp>

#include <array>
#include <vector>

namespace vcam {

class Mesh {
public:
    explicit Mesh(std::vector<glm::vec4> vertices, std::vector<std::array<std::size_t, 3>> triangles)
        : m_vertices{ vertices }, m_triangles{ triangles } { }

    std::vector<glm::vec4>& vertices() {
        return m_vertices;
    }

    std::vector<std::array<std::size_t, 3>>& triangles() {
        return m_triangles;
    }

private:
    std::vector<glm::vec4> m_vertices;
    std::vector<std::array<std::size_t, 3>> m_triangles;
};

class Model {
public:
    explicit Model(Mesh mesh)
        : m_mesh{ mesh } { }

    Mesh& mesh() {
        return m_mesh;
    }

private:
    Mesh m_mesh;
};

}
