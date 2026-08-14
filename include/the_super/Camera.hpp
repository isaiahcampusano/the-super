#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace the_super {

class OrbitCamera {
public:
    OrbitCamera();

    [[nodiscard]] glm::mat4 viewMatrix() const;
    [[nodiscard]] glm::vec3 position() const;

    void orbit(float horizontalPixels, float verticalPixels);
    void pan(float horizontalPixels, float verticalPixels);
    void zoom(float wheelOffset);
    void reset(float distance = 30.0F);

private:
    glm::vec3 target_ {0.0F, 0.0F, 0.0F};
    float yaw_ {0.0F};
    float pitch_ {0.0F};
    float distance_ {12.0F};
};

} // namespace the_super
