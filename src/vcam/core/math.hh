#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>

#include <numbers>

namespace vcam {

glm::mat4 calculate_transform_matrix(glm::vec3 const& translation, glm::vec3 const& rotation, glm::vec3 const& scale);

constexpr float to_radians(float degrees) {
    return degrees * std::numbers::pi / 180.0f;
}

}
