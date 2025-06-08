#include <vcam/core/math.hh>
#include <vcam/render/render_system.hh>

#include <algorithm>
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
    int width, height;
    SDL_GetRenderOutputSize(m_renderer, &width, &height);

    SDL_SetRenderDrawColorFloat(m_renderer, 1.0f, 1.0f, 1.0f, SDL_ALPHA_OPAQUE_FLOAT);
    SDL_RenderClear(m_renderer);

    m_surface = SDL_CreateSurface(
        width,
        height,
        SDL_PIXELFORMAT_RGBA8888
    );
    SDL_ClearSurface(m_surface, 1.0f, 1.0f, 1.0f, 1.0f);

    auto const scene_to_camera_transform = calculate_scene_to_camera_transform(m_camera);

    auto const camera_to_projection_transform = calculate_camera_to_projection_transform(m_camera, static_cast<float>(width) / height);
    auto const projection_to_camera_transform = glm::inverse(camera_to_projection_transform);

    auto const projection_to_viewport_transform = calculate_projection_to_viewport_transform(width, height);
    auto const viewport_to_projection_transform = glm::inverse(projection_to_viewport_transform);

    m_light.position = glm::vec3(scene_to_camera_transform * glm::vec4(m_light.position, 1.0f));
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

        rasterize_model(
            scratch,
            projection_to_camera_transform,
            viewport_to_projection_transform,
            depth_buffer,
            width,
            height
        );
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

    for (auto& normals : scratch.triangle_normals) {
        for (auto& normal : normals) {
            normal = glm::transpose(glm::inverse(glm::mat3(model_to_camera_transform))) * normal;
        }
    }
}

void RenderSystem::project_model(ScratchModel& scratch, glm::mat4 const& camera_to_projection_transform) {
    for (auto& vertex : scratch.vertices) {
        vertex = camera_to_projection_transform * vertex;
    }
}

void RenderSystem::clip_model(ScratchModel& scratch) {
    auto& vertices = scratch.vertices;
    auto& normals = scratch.triangle_normals;
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

    std::array<ClipPlane, 6> planes = {
        ClipPlane::LEFT, ClipPlane::RIGHT,
        ClipPlane::BOTTOM, ClipPlane::TOP,
        ClipPlane::NEAR, ClipPlane::FAR
    };

    std::vector<std::array<std::size_t, 3>> clipped_triangles;
    clipped_triangles.reserve(triangles.size() * 2);

    std::vector<std::array<glm::vec3, 3>> clipped_normals;
    clipped_normals.reserve(triangles.size() * 2);

    for (std::size_t i = 0; i < triangles.size(); ++i) {
        auto const triangle = triangles[i];
        auto const normal = normals[i];

        std::vector<std::size_t> polygon = { triangle[0], triangle[1], triangle[2] };
        std::vector<glm::vec3> polygon_normals = { normal[0], normal[1], normal[2] };

        for (ClipPlane p : planes) {
            if (polygon.empty()) break;

            std::vector<std::size_t> next_polygon;
            next_polygon.reserve(polygon.size() + 3);

            std::vector<glm::vec3> next_polygon_normals;
            next_polygon_normals.reserve(polygon.size() + 3);

            for (std::size_t i = 0; i < polygon.size(); ++i) {
                auto const i0 = polygon[i];
                auto const i1 = polygon[(i + 1) % polygon.size()];

                auto const& n0 = polygon_normals[i];
                auto const& n1 = polygon_normals[(i + 1) % polygon.size()];

                auto const d0 = clip_distance(vertices[i0], p);
                auto const d1 = clip_distance(vertices[i1], p);

                auto const in0 = d0 >= 0.f;
                auto const in1 = d1 >= 0.f;

                if (in0) {
                    next_polygon.push_back(i0);
                    next_polygon_normals.push_back(n0);
                }

                if (in0 ^ in1) {
                    float t = d0 / (d0 - d1);

                    auto const mixed_vertex = glm::mix(vertices[i0], vertices[i1], t);
                    auto const mixed_normal = glm::mix(n0, n1, t);

                    vertices.push_back(mixed_vertex);
                    next_polygon.push_back(vertices.size() - 1);
                    next_polygon_normals.push_back(mixed_normal);
                }
            }
            polygon.swap(next_polygon);
            polygon_normals.swap(next_polygon_normals);
        }

        if (polygon.size() < 3) {
            continue;
        }

        for (std::size_t i = 1; i + 1 < polygon.size(); ++i) {
            clipped_triangles.push_back({ polygon[0], polygon[i], polygon[i + 1] });
            clipped_normals.push_back({
                    polygon_normals[0],
                    polygon_normals[i],
                    polygon_normals[i + 1],
                });
        }
    }

    triangles.swap(clipped_triangles);
    normals.swap(clipped_normals);
}

void RenderSystem::normalize_model(ScratchModel& scratch) {
    for (std::size_t i = 0; i < scratch.vertices.size(); ++i) {
        auto& vertex = scratch.vertices[i];

        auto const inv_w = 1 / vertex.w;
        vertex *= inv_w;
        vertex.w = inv_w;
    }

    for (std::size_t i = 0; i < scratch.triangles.size(); ++i) {
        auto& triangle = scratch.triangles[i];
        auto& normals = scratch.triangle_normals[i];

        for (std::size_t j = 0; j < normals.size(); ++j) {
            normals[j] *= scratch.vertices[triangle[j]].w;
        }
    }
}

void RenderSystem::viewport_model(ScratchModel& scratch, glm::mat4 const& projection_to_viewport_transform) {
    for (auto& vertex : scratch.vertices) {
        auto const inv_w = vertex.w;

        vertex = projection_to_viewport_transform * glm::vec4(glm::vec3(vertex), 1.0f);
        vertex.w = inv_w;
    }
}

struct BoundingBox {
    int min_x;
    int min_y;
    int max_x;
    int max_y;
};

static BoundingBox calculate_bounding_box(
    glm::vec4 const& v0,
    glm::vec4 const& v1,
    glm::vec4 const& v2,
    int width,
    int height
);

static glm::vec3 calculate_barycentric_coordinates(
    glm::vec2 const& a,
    glm::vec2 const& b,
    glm::vec2 const& c,
    glm::vec2 const& point
);

static float calculate_depth(
    glm::vec4 const& v0,
    glm::vec4 const& v1,
    glm::vec4 const& v2,
    glm::vec3 const& lambda
);

static bool is_back_face(glm::vec4 v0, glm::vec4 v1, glm::vec4 v2);

static glm::vec3 calculate_illumination(
    glm::vec4 const& v0,
    glm::vec4 const& v1,
    glm::vec4 const& v2,
    Material const& material,
    glm::vec3 const& n0,
    glm::vec3 const& n1,
    glm::vec3 const& n2,
    glm::vec3 const& lambda,
    glm::mat4 const& projection_to_camera_transform,
    glm::mat4 const& viewport_to_projection_transform,
    Light const& light
);

template <typename T>
static T interpolate_barycentrically(
    T const& a,
    T const& b,
    T const& c,
    glm::vec3 const& lambda
);

void RenderSystem::rasterize_model(
    ScratchModel& scratch,
    glm::mat4 const& projection_to_camera_transform,
    glm::mat4 const& viewport_to_projection_transform,
    std::vector<float>& depth_buffer,
    int width,
    int height
) {
    auto const& vertices = scratch.vertices;

    for (std::size_t i = 0; i < scratch.triangles.size(); ++i) {
        auto const& triangle = scratch.triangles[i];

        if (is_back_face(vertices[triangle[0]], vertices[triangle[1]], vertices[triangle[2]])) {
            continue;
        }

        auto const bounding_box = calculate_bounding_box(vertices[triangle[0]], vertices[triangle[1]], vertices[triangle[2]], width, height);

        for (int y = bounding_box.min_y; y <= bounding_box.max_y; ++y) {
            if (y < 0 || y >= height) {
                continue;
            }

            for (int x = bounding_box.min_x; x <= bounding_box.max_x; ++x) {
                if (x < 0 || x >= width) {
                    continue;
                }

                auto const lambda = calculate_barycentric_coordinates(
                    vertices[triangle[0]], vertices[triangle[1]], vertices[triangle[2]],
                    glm::vec2(x + 0.5f, y + 0.5f)
                );

                auto const depth = calculate_depth(
                    vertices[triangle[0]],
                    vertices[triangle[1]],
                    vertices[triangle[2]],
                    lambda
                );

                auto const depth_buffer_index = static_cast<std::size_t>(y) * width + x;
                if (std::isnan(depth) || depth <= depth_buffer[depth_buffer_index]) {
                    continue;
                }

                auto const& triangle_normals = scratch.triangle_normals[i];
                auto const& material = scratch.model.material();

                auto const illumination = calculate_illumination(
                    vertices[triangle[0]], vertices[triangle[1]], vertices[triangle[2]],
                    material,
                    triangle_normals[0], triangle_normals[1], triangle_normals[2],
                    lambda,
                    projection_to_camera_transform,
                    viewport_to_projection_transform,
                    m_light
                );

                auto linear_color = material.color() * illumination;
                auto const srgb_color = glm::pow(linear_color, glm::vec3(1.0f / 2.2f));

                depth_buffer[depth_buffer_index] = depth;
                SDL_WriteSurfacePixelFloat(m_surface, x, y, srgb_color.r, srgb_color.g, srgb_color.b, SDL_ALPHA_OPAQUE);
            }
        }
    }
}

BoundingBox calculate_bounding_box(
    glm::vec4 const& v0,
    glm::vec4 const& v1,
    glm::vec4 const& v2,
    int width,
    int height
) {
    auto const min_x = std::clamp(static_cast<int>(std::floor(std::min({ v0.x, v1.x, v2.x }))), 0, width);
    auto const min_y = std::clamp(static_cast<int>(std::floor(std::min({ v0.y, v1.y, v2.y }))), 0, height);
    auto const max_x = std::clamp(static_cast<int>(std::ceil(std::max({ v0.x, v1.x, v2.x }))), 0, width);
    auto const max_y = std::clamp(static_cast<int>(std::ceil(std::max({ v0.y, v1.y, v2.y }))), 0, height);

    return { min_x, min_y, max_x, max_y };
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

float calculate_depth(
    glm::vec4 const& v0,
    glm::vec4 const& v1,
    glm::vec4 const& v2,
    glm::vec3 const& lambda
) {
    auto const inv_w = interpolate_barycentrically(v0.w, v1.w, v2.w, lambda);
    auto const reverse_z = interpolate_barycentrically(v0.z, v1.z, v2.z, lambda) / inv_w;
    return reverse_z;
}


bool is_back_face(glm::vec4 v0, glm::vec4 v1, glm::vec4 v2) {
    auto const direction = glm::cross(
        glm::vec3(glm::vec2(v1), 0.0f) - glm::vec3(glm::vec2(v0), 0.0f),
        glm::vec3(glm::vec2(v2), 0.0f) - glm::vec3(glm::vec2(v0), 0.0f)
    ).z;
    return direction > 0;
}

static glm::vec3 calculate_illumination(
    glm::vec4 const& v0,
    glm::vec4 const& v1,
    glm::vec4 const& v2,
    Material const& material,
    glm::vec3 const& n0,
    glm::vec3 const& n1,
    glm::vec3 const& n2,
    glm::vec3 const& lambda,
    glm::mat4 const& projection_to_camera_transform,
    glm::mat4 const& viewport_to_projection_transform,
    Light const& light
) {
    auto const inv_w = interpolate_barycentrically(v0.w, v1.w, v2.w, lambda);

    auto const normal = glm::normalize(interpolate_barycentrically(n0, n1, n2, lambda) / inv_w);

    auto const viewport_position = interpolate_barycentrically(v0, v1, v2, lambda);

    auto const normalized_position = viewport_to_projection_transform * glm::vec4(glm::vec3(viewport_position), 1.0f);
    auto const clip_position = normalized_position / viewport_position.w;
    auto const position = glm::vec3(projection_to_camera_transform * clip_position);

    auto const camera_position = glm::vec3(0.0f, 0.0f, 0.0f);
    auto const light_position = light.position;

    auto const L = glm::normalize(light_position - position);
    auto const R = glm::normalize(2 * glm::dot(L, normal) * normal - L);
    auto const V = glm::normalize(camera_position - position);

    auto const ambient_intensity = glm::vec3(0.2f);
    auto const specular_intensity = glm::vec3(1.0f);
    auto const diffuse_intensity = glm::vec3(0.8f);

    auto const ambient_illumination = material.ambient_reflection() * ambient_intensity;
    auto const diffuse_illumination = material.diffuse_reflection() * glm::dot(L, normal) * diffuse_intensity;
    auto const specular_illumination = material.specular_reflection() *
        std::pow(std::max(glm::dot(R, V), 0.0f), material.shininess()) * specular_intensity;
    auto const illumination = ambient_illumination + diffuse_illumination + specular_illumination;

    return illumination;
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
