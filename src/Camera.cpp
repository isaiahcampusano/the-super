#include "the_super/Camera.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>

namespace the_super {

namespace {

constexpr float orbitSensitivity = 0.005F;
constexpr float panSensitivity = 0.0015F;
constexpr float maximumPitch = std::numbers::pi_v<float> * 0.49F;

} // namespace

OrbitCamera::OrbitCamera() {
    reset();
}

glm::vec3 OrbitCamera::position() const {
    const float horizontalScale = std::cos(pitch_);
    const glm::vec3 offset {
        horizontalScale * std::cos(yaw_),
        horizontalScale * std::sin(yaw_),
        std::sin(pitch_),
    };
    return target_ + (offset * distance_);
}

glm::mat4 OrbitCamera::viewMatrix() const {
    return glm::lookAt(position(), target_, glm::vec3 {0.0F, 0.0F, 1.0F});
}

void OrbitCamera::orbit(float horizontalPixels, float verticalPixels) {
    yaw_ -= horizontalPixels * orbitSensitivity;
    pitch_ += verticalPixels * orbitSensitivity;
    pitch_ = std::clamp(pitch_, -maximumPitch, maximumPitch);
}

void OrbitCamera::pan(float horizontalPixels, float verticalPixels) {
    const glm::vec3 forward = glm::normalize(target_ - position());
    const glm::vec3 right = glm::normalize(
        glm::cross(forward, glm::vec3 {0.0F, 0.0F, 1.0F})
    );
    const glm::vec3 up = glm::normalize(glm::cross(right, forward));
    const float scale = distance_ * panSensitivity;
    target_ += ((-right * horizontalPixels) + (up * verticalPixels)) * scale;
}

void OrbitCamera::zoom(float wheelOffset) {
    distance_ *= std::exp(-wheelOffset * 0.12F);
    distance_ = std::clamp(distance_, 1.25F, 80.0F);
}

void OrbitCamera::reset(float distance) {
    target_ = glm::vec3 {0.0F, 0.0F, 0.0F};
    yaw_ = std::numbers::pi_v<float> * 0.10F;
    pitch_ = std::numbers::pi_v<float> * 0.08F;
    distance_ = std::clamp(distance, 1.25F, 80.0F);
}

} // namespace the_super
