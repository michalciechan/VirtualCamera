#pragma once

#include <vcam/core/scene.hh>
#include <vcam/render/model.hh>

#include <SDL3/SDL_render.h>
#include <glm/glm.hpp>

#include <unordered_map>
#include <vector>

namespace vcam {

struct Camera {
    glm::vec3 position;
    glm::vec3 rotation;
    float vfov;
};

enum ClipPlane {
    LEFT,
    RIGHT,
    BOTTOM,
    TOP,
    NEAR,
    FAR
};

class RenderSystem {
public:
    explicit RenderSystem(SDL_Renderer* renderer)
        : m_renderer(renderer) { }

    void render();

    void camera(Camera camera) {
        m_camera = camera;
    }

    void add_instance(Model const& model, glm::mat4 model_to_scene_transform);

private:
    SDL_Renderer* m_renderer;
    Camera m_camera;

    std::unordered_multimap<Model const*, glm::mat4> m_models;

    glm::mat4 calculate_scene_to_camera_transform(Camera const& camera);
    glm::mat4 calculate_camera_to_projection_transform(Camera const& camera, float aspect_ratio);
    glm::mat4 calculate_projection_to_viewport_transform(int width, int height);

    Model transform_model(Model model, glm::mat4 const& model_to_camera_transform);
    Model project_model(Model model, glm::mat4 const& camera_to_projection_transform);
    Model clip_model(Model model);
    std::vector<std::array<glm::vec4, 3>> clip_triangle(std::array<glm::vec4, 3> vertices);
    std::vector<std::array<glm::vec4, 3>> clip_triangle_against_clip_plane(std::array<glm::vec4, 3> vertices, ClipPlane plane);
    Model normalize_model(Model model);

    void render_model(Model model, glm::mat4 const& projection_to_viewport_transform);
};

}
