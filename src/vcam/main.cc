#include <vcam/core/entity.hh>
#include <vcam/core/math.hh>
#include <vcam/movement/movement_controller.hh>
#include <vcam/render/model.hh>
#include <vcam/render/camera_component.hh>
#include <vcam/render/light_component.hh>
#include <vcam/render/render_component.hh>
#include <vcam/render/render_system.hh>
#include <vcam/core/scene.hh>

#include <glm/glm.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cmath>
#include <memory>
#include <random>
#include <string>

struct GlobalState {
    bool is_running;
    vcam::RenderSystem render_system;
    vcam::Scene scene;
};

void on_init(GlobalState& state);
void on_update(GlobalState& state, float dt);
void on_shutdown(GlobalState& state);

int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION,
            "Failed to initialize SDL: %s",
            SDL_GetError()
        );
        return 1;
    }

    auto* const window = SDL_CreateWindow(
        "VirtualCamera",
        1024,
        768,
        SDL_WINDOW_RESIZABLE
    );
    if (window == nullptr) {
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION,
            "Failed to create window: %s",
            SDL_GetError()
        );
        SDL_Quit();
        return 1;
    }

    auto* const renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer == nullptr) {
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION,
            "Failed to create renderer: %s",
            SDL_GetError()
        );
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    GlobalState state{
        .is_running = true,
        .render_system = vcam::RenderSystem(renderer),
    };

    on_init(state);

    constexpr std::uint32_t MILLISECONDS_PER_SECOND = 1000;
    constexpr std::uint32_t FRAMES_PER_SECOND = 60;

    auto const target = MILLISECONDS_PER_SECOND / FRAMES_PER_SECOND;
    auto last_ticks = SDL_GetTicks();

    while (state.is_running) {
        auto current_ticks = SDL_GetTicks();

        auto const dt = current_ticks - last_ticks;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_EVENT_QUIT:
                state.is_running = false;
                break;

            default:
                break;
            }
        }

        on_update(state, dt);

        last_ticks = current_ticks;

        current_ticks = SDL_GetTicks();
        auto const elapsed = current_ticks - last_ticks;

        if (elapsed < target) {
            SDL_Delay(target - elapsed);
        }
    }

    on_shutdown(state);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

static vcam::Mesh generate_sphere_mesh(std::size_t subdivisions);
static vcam::Material create_gold_material();
static vcam::Material create_plastic_material();

void on_init(GlobalState& state) {
    auto const mesh = generate_sphere_mesh(3);
    auto model = std::make_shared<vcam::Model>(
        mesh,
        create_gold_material()
    );

    auto entity = std::make_shared<vcam::Entity>();
    entity->position(glm::vec3(-2.0f, 0.0f, 0.0f));
    entity->add_component(
        std::make_unique<vcam::RenderComponent>(state.render_system, model)
    );
    state.scene.add_entity(entity);

    model = std::make_shared<vcam::Model>(
        mesh,
        create_plastic_material()
    );
    entity = std::make_shared<vcam::Entity>();
    entity->position(glm::vec3(2.0f, 0.0f, 0.0f));
    entity->add_component(
        std::make_unique<vcam::RenderComponent>(state.render_system, model)
    );
    state.scene.add_entity(entity);

    auto light_entity = std::make_shared<vcam::Entity>();
    light_entity->add_component(
        std::make_unique<vcam::LightComponent>(
            state.render_system,
            glm::vec3(0.2f),
            glm::vec3(1.0f),
            glm::vec3(0.8f)
        ));
    state.scene.add_entity(light_entity);

    auto camera_entity = std::make_shared<vcam::Entity>();
    camera_entity->position(glm::vec3(0.0f, 0.0f, -10.0f));
    camera_entity->add_component(std::make_unique<vcam::MovementController>());
    camera_entity->add_component(std::make_unique<vcam::CameraComponent>(state.render_system));
    state.scene.add_entity(camera_entity);

    on_update(state, 0.0f);
}

void on_update(GlobalState& state, float dt) {
    for (auto entity : state.scene.entities()) {
        entity->on_update(dt);
    }

    state.render_system.render();
}

void on_shutdown(GlobalState& state) { }

static vcam::Mesh generate_icosahedron_mesh();

vcam::Mesh generate_sphere_mesh(std::size_t subdivisions) {
    auto const mesh = generate_icosahedron_mesh();

    std::vector<glm::vec3> vertices(mesh.vertices());
    std::vector<std::array<std::size_t, 3>> triangles(mesh.triangles());
    std::vector<std::array<glm::vec3, 3>> triangle_normals(mesh.triangle_normals());

    for (auto i = 0; i < subdivisions; ++i) {
        auto next_vertices = vertices;
        std::vector<std::array<std::size_t, 3>> next_triangles;
        std::vector<std::array<glm::vec3, 3>> next_triangle_normals;

        for (auto& triangle : triangles) {
            auto const& v0 = vertices[triangle[0]];
            auto const& v1 = vertices[triangle[1]];
            auto const& v2 = vertices[triangle[2]];

            auto const mid01 = glm::normalize((v0 + v1) * 0.5f);
            auto const mid12 = glm::normalize((v1 + v2) * 0.5f);
            auto const mid20 = glm::normalize((v2 + v0) * 0.5f);

            next_vertices.push_back(mid01);
            next_vertices.push_back(mid12);
            next_vertices.push_back(mid20);

            auto const index01 = next_vertices.size() - 3;
            auto const index12 = next_vertices.size() - 2;
            auto const index20 = next_vertices.size() - 1;

            next_triangles.push_back({ triangle[0], index01, index20 });
            next_triangles.push_back({ triangle[1], index12, index01 });
            next_triangles.push_back({ triangle[2], index20, index12 });
            next_triangles.push_back({ index01, index12, index20 });

            auto const normal01 = glm::normalize(mid01);
            auto const normal12 = glm::normalize(mid12);
            auto const normal20 = glm::normalize(mid20);
            next_triangle_normals.push_back({ v0, normal01, normal20 });
            next_triangle_normals.push_back({ v1, normal12, normal01 });
            next_triangle_normals.push_back({ v2, normal20, normal12 });
            next_triangle_normals.push_back({ normal01, normal12, normal20 });
        }

        vertices = next_vertices;
        triangles = next_triangles;
        triangle_normals = next_triangle_normals;
    }

    return vcam::Mesh(
        vertices,
        triangles,
        triangle_normals
    );
}

vcam::Mesh generate_icosahedron_mesh() {
    float phi = (1.0f + sqrt(5.0f)) * 0.5f;
    float a = 1.0f;
    float b = 1.0f / phi;

    std::vector<glm::vec3> vertices;
    std::vector<std::array<std::size_t, 3>> triangles;
    std::vector<std::array<glm::vec3, 3>> normals;

    vertices.emplace_back(0, b, -a);
    vertices.emplace_back(b, a, 0);
    vertices.emplace_back(-b, a, 0);
    vertices.emplace_back(0, b, a);
    vertices.emplace_back(0, -b, a);
    vertices.emplace_back(-a, 0, b);
    vertices.emplace_back(0, -b, -a);
    vertices.emplace_back(a, 0, -b);
    vertices.emplace_back(a, 0, b);
    vertices.emplace_back(-a, 0, -b);
    vertices.emplace_back(b, -a, 0);
    vertices.emplace_back(-b, -a, 0);

    for (auto& vertex : vertices) {
        auto const normal = glm::normalize(vertex);
        vertex = normal;
    }

    triangles.push_back({ 0, 1, 2 });
    triangles.push_back({ 3, 2, 1 });
    triangles.push_back({ 3, 4, 5 });
    triangles.push_back({ 3, 8, 4 });
    triangles.push_back({ 0, 6, 7 });
    triangles.push_back({ 0, 9, 6 });
    triangles.push_back({ 4, 10, 11 });
    triangles.push_back({ 6, 11, 10 });
    triangles.push_back({ 2, 5, 9 });
    triangles.push_back({ 11, 9, 5 });
    triangles.push_back({ 1, 7, 8 });
    triangles.push_back({ 10, 8, 7 });
    triangles.push_back({ 3, 5, 2 });
    triangles.push_back({ 3, 1, 8 });
    triangles.push_back({ 0, 2, 9 });
    triangles.push_back({ 0, 7, 1 });
    triangles.push_back({ 6, 9, 11 });
    triangles.push_back({ 6, 10, 7 });
    triangles.push_back({ 4, 11, 5 });
    triangles.push_back({ 4, 8, 10 });

    for (auto const& triangle : triangles) {
        normals.push_back({
                vertices[triangle[0]],
                vertices[triangle[1]],
                vertices[triangle[2]],
            });
    }

    return vcam::Mesh(vertices, triangles, normals);
}

vcam::Material create_gold_material() {
    return vcam::Material(
        glm::vec3(1.0f, 0.843f, 0.0f),
        100.0f,
        glm::vec3(0.628f, 0.555f, 0.366f),
        glm::vec3(0.75164f, 0.60648f, 0.22648f),
        glm::vec3(0.24725f, 0.1995f, 0.0745f)
    );
}

vcam::Material create_plastic_material() {
    return vcam::Material(
        glm::vec3(0.8f, 0.1f, 0.1f),
        10.0f,
        glm::vec3(0.5f, 0.5f, 0.5f),
        glm::vec3(0.8f, 0.1f, 0.1f),
        glm::vec3(0.1f, 0.01f, 0.01f)
    );
}
