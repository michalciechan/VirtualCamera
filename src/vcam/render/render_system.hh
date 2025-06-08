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

struct Light {
    glm::vec3 position;
    glm::vec3 ambient_intensity;
    glm::vec3 specular_intensity;
    glm::vec3 diffuse_intensity;
};

enum ClipPlane {
    LEFT,
    RIGHT,
    BOTTOM,
    TOP,
    NEAR,
    FAR
};

struct ScratchModel {
    Model const& model;
    std::vector<glm::vec4> vertices;
    std::vector<std::array<std::size_t, 3>> triangles;
    std::vector<std::array<glm::vec3, 3>> triangle_normals;

    std::vector<float> depths;

    ScratchModel(Model const& model)
        : model(model),
        triangles(model.mesh().triangles()),
        triangle_normals(model.mesh().triangle_normals()) {
        vertices.reserve(model.mesh().vertices().size());
        for (auto const& vertex : model.mesh().vertices()) {
            vertices.emplace_back(vertex, 1.0f);
        }
    }
};

class RenderSystem {
public:
    explicit RenderSystem(SDL_Renderer* renderer)
        : m_renderer(renderer) { }

    void render();

    void camera(Camera camera) {
        m_camera = camera;
    }

    void light(Light light) {
        m_light = light;
    }

    void add_instance(Model const& model, glm::mat4 model_to_scene_transform);

private:
    SDL_Renderer* m_renderer;
    Camera m_camera;
    Light m_light;

    std::unordered_multimap<Model const*, glm::mat4> m_models;

    void transform_model(ScratchModel& scratch, glm::mat4 const& model_to_camera_transform);
    void project_model(ScratchModel& scratch, glm::mat4 const& camera_to_projection_transform);
    void clip_model(ScratchModel& scratch);
    void normalize_model(ScratchModel& scratch);
    void viewport_model(ScratchModel& scratch, glm::mat4 const& projection_to_viewport_transform);

    void rasterize_model(
        ScratchModel& scratch,
        glm::mat4 const& projection_to_camera_transform,
        glm::mat4 const& viewport_to_projection_transform,
        std::vector<float>& z_buffer,
        int width,
        int height
    );

    SDL_Surface* m_surface = nullptr;
};

}
