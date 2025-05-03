#include <vcam/core/math.hh>

glm::mat4 vcam::calculate_transform_matrix(glm::vec3 const& translation, glm::vec3 const& rotation, glm::vec3 const& scale) {
    auto const rotation_matrix = glm::yawPitchRoll(rotation.y, rotation.x, rotation.z);
    auto const translation_matrix = glm::translate(translation);
    auto const scale_matrix = glm::scale(scale);
    return translation_matrix * rotation_matrix * scale_matrix;
}
