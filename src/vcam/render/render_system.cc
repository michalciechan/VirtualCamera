#include <vcam/core/math.hh>
#include <vcam/render/render_system.hh>

#include <cmath>
#include <utility>

namespace vcam {

void RenderSystem::add_instance(Model const& model, glm::mat4 model_to_scene_transform) {
    m_models.insert(std::make_pair(&model, model_to_scene_transform));
}

static glm::mat4 calculate_scene_to_camera_transform(Camera const& camera);
static glm::mat4 calculate_camera_to_projection_transform(Camera const& camera, float aspect_ratio);
static glm::mat4 calculate_projection_to_viewport_transform(int width, int height);

void RenderSystem::render() {
    auto const scene_to_camera_transform = calculate_scene_to_camera_transform(m_camera);

    int width, height;
    SDL_GetRenderOutputSize(m_renderer, &width, &height);

    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(m_renderer);

    m_surface = SDL_CreateSurface(
        width,
        height,
        SDL_PIXELFORMAT_RGBA8888
    );

    auto const camera_to_projection_transform = calculate_camera_to_projection_transform(m_camera, static_cast<float>(width) / height);
    auto const projection_to_viewport_transform = calculate_projection_to_viewport_transform(width, height);

    std::vector<float> depth_buffer(
        width * height,
        -std::numeric_limits<float>::infinity()
    );

    for (auto const& [model, model_to_scene_transform] : m_models) {
        auto const model_to_camera_transform = scene_to_camera_transform * model_to_scene_transform;

        ScratchModel scratch(*model);

        transform_model(scratch, model_to_camera_transform);
        project_model(scratch, camera_to_projection_transform);
        clip_model(scratch);
        normalize_model(scratch);
        viewport_model(scratch, projection_to_viewport_transform);

        rasterize_model(scratch, depth_buffer, width, height);
    }

    auto* const texture = SDL_CreateTextureFromSurface(
        m_renderer,
        m_surface
    );
    SDL_RenderTexture(m_renderer, texture, nullptr, nullptr);
    SDL_DestroyTexture(texture);
    SDL_RenderPresent(m_renderer);

    SDL_DestroySurface(m_surface);
    m_surface = nullptr;

    m_models.clear();
}

glm::mat4 calculate_scene_to_camera_transform(Camera const& camera) {
    auto const camera_to_scene_transform = calculate_transform_matrix(
        camera.position,
        camera.rotation,
        glm::vec3(1.0f)
    );

    return glm::inverse(camera_to_scene_transform);
}

glm::mat4 calculate_camera_to_projection_transform(Camera const& camera, float aspect_ratio) {
    auto const half_tan = std::tan(to_radians(camera.vfov) * 0.5f);
    auto const z_near = 0.01f;
    auto const z_far = 1000.0f;

    return {
        1 / (half_tan * aspect_ratio), 0, 0, 0,
        0, 1 / half_tan, 0, 0,
        0, 0, z_near / (z_near - z_far), 1,
        0, 0, (z_near * z_far) / (z_far - z_near), 0
    };
}

glm::mat4 calculate_projection_to_viewport_transform(int width, int height) {
    return {
        width / 2.0f, 0, 0, 0,
        0, -height / 2.0f, 0, 0,
        0, 0, 1, 0,
        width / 2.0f, height / 2.0f, 0, 1
    };
}

void RenderSystem::transform_model(ScratchModel& scratch, glm::mat4 const& model_to_camera_transform) {
    for (auto& vertex : scratch.vertices) {
        vertex = model_to_camera_transform * vertex;
    }
}

void RenderSystem::project_model(ScratchModel& scratch, glm::mat4 const& camera_to_projection_transform) {
    for (auto& vertex : scratch.vertices) {
        vertex = camera_to_projection_transform * vertex;
    }
}

void RenderSystem::clip_model(ScratchModel& scratch) {
    auto& vertices = scratch.vertices;
    auto& colors = scratch.colors;
    auto& inv_w = scratch.inv_w;
    auto& triangles = scratch.triangles;

    auto clip_distance = [](glm::vec4 const& v, ClipPlane p) -> float {
        switch (p) {
        case ClipPlane::LEFT:   return v.x + v.w;
        case ClipPlane::RIGHT:  return v.w - v.x;
        case ClipPlane::BOTTOM: return v.y + v.w;
        case ClipPlane::TOP:    return v.w - v.y;
        case ClipPlane::NEAR:   return v.z + v.w;
        case ClipPlane::FAR:    return v.w - v.z;
        }
        return 0.f;
        };

    auto push_vertex = [&](
        glm::vec4 const& pos,
        glm::vec3 const& col,
        float invw) -> std::size_t {
            vertices.push_back(pos);
            colors.push_back(col);
            inv_w.push_back(invw);
            return vertices.size() - 1;
        };

    std::array<ClipPlane, 6> planes = {
        ClipPlane::LEFT, ClipPlane::RIGHT,
        ClipPlane::BOTTOM, ClipPlane::TOP,
        ClipPlane::NEAR, ClipPlane::FAR
    };

    std::vector<std::array<std::size_t, 3>> clipped_triangles;
    clipped_triangles.reserve(triangles.size() * 2);

    for (auto const& triangle : triangles) {
        std::vector<std::size_t> polygon = { triangle[0], triangle[1], triangle[2] };

        for (ClipPlane p : planes) {
            if (polygon.empty()) break;

            std::vector<std::size_t> next_polygon;
            next_polygon.reserve(polygon.size() + 3);

            for (std::size_t i = 0; i < polygon.size(); ++i) {
                std::size_t i0 = polygon[i];
                std::size_t i1 = polygon[(i + 1) % polygon.size()];

                float d0 = clip_distance(vertices[i0], p);
                float d1 = clip_distance(vertices[i1], p);

                bool in0 = d0 >= 0.f;
                bool in1 = d1 >= 0.f;

                if (in0) next_polygon.push_back(i0);

                if (in0 ^ in1) {
                    float t = d0 / (d0 - d1);

                    glm::vec4 pos = glm::mix(vertices[i0], vertices[i1], t);
                    glm::vec3 col = glm::mix(colors[i0], colors[i1], t);
                    float iw = glm::mix(inv_w[i0], inv_w[i1], t);

                    next_polygon.push_back(push_vertex(pos, col, iw));
                }
            }
            polygon.swap(next_polygon);
        }

        if (polygon.size() < 3) continue;

        for (std::size_t i = 1; i + 1 < polygon.size(); ++i)
            clipped_triangles.push_back({ polygon[0], polygon[i], polygon[i + 1] });
    }

    triangles.swap(clipped_triangles);
}

void RenderSystem::normalize_model(ScratchModel& scratch) {
    for (std::size_t i = 0; i < scratch.vertices.size(); ++i) {
        auto& vertex = scratch.vertices[i];

        auto const inv_w = 1 / vertex.w;
        vertex *= inv_w;
        vertex.w = inv_w;
    }
}

void RenderSystem::viewport_model(ScratchModel& scratch, glm::mat4 const& projection_to_viewport_transform) {
    for (auto& vertex : scratch.vertices) {
        auto const inv_w = vertex.w;

        vertex = projection_to_viewport_transform * glm::vec4(glm::vec3(vertex), 1.0f);
        vertex.w = inv_w;
    }
}

static glm::vec3 calculate_barycentric_coordinates(
    glm::vec2 const& a,
    glm::vec2 const& b,
    glm::vec2 const& c,
    glm::vec2 const& point
);

static bool is_back_face(glm::vec4 v0, glm::vec4 v1, glm::vec4 v2);

template <typename T>
static T interpolate_barycentrically(
    T const& a,
    T const& b,
    T const& c,
    glm::vec3 const& lambda
);

void RenderSystem::rasterize_model(
    ScratchModel& scratch,
    std::vector<float>& depth_buffer,
    int width,
    int height
) {
    auto const& vertices = scratch.vertices;

    for (auto const& triangle : scratch.triangles) {
        if (is_back_face(vertices[triangle[0]], vertices[triangle[1]], vertices[triangle[2]])) {
            continue;
        }

        auto const min_x = std::min({ vertices[triangle[0]].x, vertices[triangle[1]].x, vertices[triangle[2]].x });
        auto const max_x = std::max({ vertices[triangle[0]].x, vertices[triangle[1]].x, vertices[triangle[2]].x });
        auto const min_y = std::min({ vertices[triangle[0]].y, vertices[triangle[1]].y, vertices[triangle[2]].y });
        auto const max_y = std::max({ vertices[triangle[0]].y, vertices[triangle[1]].y, vertices[triangle[2]].y });

        for (int y = std::floor(min_y); y <= std::ceil(max_y); ++y) {
            if (y < 0 || y >= height) {
                continue;
            }

            for (int x = std::floor(min_x); x <= std::ceil(max_x); ++x) {
                if (x < 0 || x >= width) {
                    continue;
                }

                auto const lambda = calculate_barycentric_coordinates(
                    vertices[triangle[0]], vertices[triangle[1]], vertices[triangle[2]],
                    glm::vec2(x + 0.5f, y + 0.5f)
                );

                auto const inv_w = interpolate_barycentrically(
                    vertices[triangle[0]].w,
                    vertices[triangle[1]].w,
                    vertices[triangle[2]].w,
                    lambda
                );

                auto const reverse_z = interpolate_barycentrically(
                    vertices[triangle[0]].z,
                    vertices[triangle[1]].z,
                    vertices[triangle[2]].z,
                    lambda
                ) / inv_w;

                auto const depth_buffer_index = static_cast<std::size_t>(y) * width + x;


                if (std::isnan(reverse_z) || reverse_z > depth_buffer[depth_buffer_index]) {
                    auto color = scratch.colors[triangle[0]] * lambda.x +
                        scratch.colors[triangle[1]] * lambda.y +
                        scratch.colors[triangle[2]] * lambda.z;
                    color /= depth;
                    SDL_WriteSurfacePixelFloat(m_surface, x, y, color.r, color.g, color.b, SDL_ALPHA_OPAQUE);
                }
            }
        }
    }
}

glm::vec3 calculate_barycentric_coordinates(
    glm::vec2 const& a,
    glm::vec2 const& b,
    glm::vec2 const& c,
    glm::vec2 const& point
) {
    auto const edge = [](glm::vec2 const& a, glm::vec2 const& b, glm::vec2 const& c) {
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        };

    auto const area = edge(a, b, c);
    auto const alpha = edge(point, b, c) / area;
    auto const beta = edge(a, point, c) / area;
    auto const gamma = edge(a, b, point) / area;

    if (alpha < 0 || beta < 0 || gamma < 0) {
        return glm::vec3(std::numeric_limits<float>::quiet_NaN());
    }

    return { alpha, beta, gamma };
}

bool is_back_face(glm::vec4 v0, glm::vec4 v1, glm::vec4 v2) {
    auto const direction = glm::cross(
        glm::vec3(glm::vec2(v1), 0.0f) - glm::vec3(glm::vec2(v0), 0.0f),
        glm::vec3(glm::vec2(v2), 0.0f) - glm::vec3(glm::vec2(v0), 0.0f)
    ).z;
    return direction > 0;
}

template <typename T>
static T interpolate_barycentrically(
    T const& a,
    T const& b,
    T const& c,
    glm::vec3 const& lambda
) {
    return a * lambda.x + b * lambda.y + c * lambda.z;
}

}
