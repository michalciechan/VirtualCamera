#include <vcam/core/math.hh>
#include <vcam/render/render_system.hh>

#include <cmath>
#include <utility>

namespace vcam {

void RenderSystem::add_instance(Model const& model, glm::mat4 model_to_scene_transform) {
    m_models.insert(std::make_pair(&model, model_to_scene_transform));
}

void RenderSystem::render() {
    auto const scene_to_camera_transform = calculate_scene_to_camera_transform(m_camera);

    int width, height;
    SDL_GetRenderOutputSize(m_renderer, &width, &height);

    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(m_renderer);

    auto const camera_to_projection_transform = calculate_camera_to_projection_transform(m_camera, static_cast<float>(width) / height);
    auto const projection_to_viewport_transform = calculate_projection_to_viewport_transform(width, height);

    for (auto const& [model, model_to_scene_transform] : m_models) {
        auto const model_to_camera_transform = scene_to_camera_transform * model_to_scene_transform;

        auto const transformed = transform_model(*model, model_to_camera_transform);
        auto const projected = project_model(transformed, camera_to_projection_transform);
        auto const clipped = clip_model(projected);
        auto const normalized = normalize_model(clipped);

        render_model(normalized, projection_to_viewport_transform);
    }

    SDL_RenderPresent(m_renderer);
    m_models.clear();
}

glm::mat4 RenderSystem::calculate_scene_to_camera_transform(Camera const& camera) {
    auto const camera_to_scene_transform = calculate_transform_matrix(
        camera.position,
        camera.rotation,
        glm::vec3(1.0f)
    );

    return glm::inverse(camera_to_scene_transform);
}

glm::mat4 RenderSystem::calculate_camera_to_projection_transform(Camera const& camera, float aspect_ratio) {
    auto const half_tan = std::tan(to_radians(camera.vfov) * 0.5f);
    auto const z_near = 1.0f;
    auto const z_far = 1000.0f;

    return {
        1 / (half_tan * aspect_ratio), 0, 0, 0,
        0, 1 / half_tan, 0, 0,
        0, 0, (-z_near - z_far) / (z_near - z_far), 1,
        0, 0, (2 * z_near * z_far) / (z_near - z_far), 0
    };
}

glm::mat4 RenderSystem::calculate_projection_to_viewport_transform(int width, int height) {
    return {
        width / 2.0f, 0, 0, 0,
        0, -height / 2.0f, 0, 0,
        0, 0, 1, 0,
        width / 2.0f, height / 2.0f, 0, 1
    };
}

Model RenderSystem::transform_model(Model model, glm::mat4 const& model_to_camera_transform) {
    auto& mesh = model.mesh();
    for (auto& vertex : mesh.vertices()) {
        vertex = model_to_camera_transform * vertex;
    }

    return model;
}

Model RenderSystem::project_model(Model model, glm::mat4 const& camera_to_projection_transform) {
    auto& mesh = model.mesh();
    auto& vertices = mesh.vertices();
    for (auto& vertex : vertices) {
        vertex = camera_to_projection_transform * vertex;
    }

    return model;
}

Model RenderSystem::clip_model(Model model) {
    auto& mesh = model.mesh();
    auto& vertices = mesh.vertices();
    auto& triangles = mesh.triangles();

    std::vector<glm::vec4> clipped_vertices;
    clipped_vertices.reserve(vertices.size());
    std::vector<std::array<std::size_t, 3>> clipped_triangles;
    clipped_triangles.reserve(triangles.size());

    for (auto& triangle : triangles) {
        auto const clipped = clip_triangle({ vertices[triangle[0]], vertices[triangle[1]], vertices[triangle[2]] });
        for (auto& triangle : clipped) {
            // TODO: Don't copy vertices unnecessarily.
            clipped_vertices.insert(clipped_vertices.end(), triangle.begin(), triangle.end());

            auto const size = clipped_vertices.size();
            clipped_triangles.push_back({ size - 3, size - 2, size - 1 });
        }
    }

    vertices = clipped_vertices;
    triangles = clipped_triangles;

    return model;
}

std::vector<std::array<glm::vec4, 3>> RenderSystem::clip_triangle(std::array<glm::vec4, 3> vertices) {
    std::array<ClipPlane, 6> clip_planes = {
        ClipPlane::LEFT,
        ClipPlane::RIGHT,
        ClipPlane::BOTTOM,
        ClipPlane::TOP,
        ClipPlane::NEAR,
        ClipPlane::FAR
    };

    std::vector<std::array<glm::vec4, 3>> result;
    result.push_back(vertices);

    for (auto& plane : clip_planes) {
        std::vector<std::array<glm::vec4, 3>> temp;

        for (auto triangle : result) {
            auto const clipped = clip_triangle_against_clip_plane(triangle, plane);
            temp.insert(temp.end(), clipped.begin(), clipped.end());
        }

        result = std::move(temp);
    }

    return result;
}

std::vector<std::array<glm::vec4, 3>> RenderSystem::clip_triangle_against_clip_plane(std::array<glm::vec4, 3> vertices, ClipPlane plane) {
    auto clip_distance = [](glm::vec4 const& v, ClipPlane plane) -> float {
        switch (plane) {
        case ClipPlane::LEFT:   return v.x + v.w;
        case ClipPlane::RIGHT:  return v.w - v.x;
        case ClipPlane::BOTTOM: return v.y + v.w;
        case ClipPlane::TOP:    return v.w - v.y;
        case ClipPlane::NEAR:   return v.z + v.w;
        case ClipPlane::FAR:    return v.w - v.z;
        }
        return 0.0f;
        };

    std::vector<glm::vec4> inside;
    std::vector<glm::vec4> outside;
    for (auto vertex : vertices) {
        if (clip_distance(vertex, plane) >= 0.0f) {
            inside.push_back(vertex);
        } else {
            outside.push_back(vertex);
        }
    }

    if (inside.size() == 0) {
        return {};
    }

    return { vertices };
}

Model RenderSystem::normalize_model(Model model) {
    auto& mesh = model.mesh();
    auto& vertices = mesh.vertices();
    for (auto& vertex : vertices) {
        vertex /= vertex.w;
    }

    return model;
}

void RenderSystem::render_model(Model model, glm::mat4 const& projection_to_viewport_transform) {
    auto& mesh = model.mesh();
    auto& vertices = mesh.vertices();
    for (auto& vertex : vertices) {
        vertex = projection_to_viewport_transform * vertex;
    }

    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);

    for (auto const& triangle : mesh.triangles()) {
        std::array<SDL_FPoint, 4> points;
        for (int i = 0; i < triangle.size(); ++i) {
            auto vertex = vertices[triangle[i]];

            points[i].x = vertex.x;
            points[i].y = vertex.y;
        }
        points[3] = points[0];

        SDL_RenderLines(m_renderer, points.data(), points.size());
    }
}

}
