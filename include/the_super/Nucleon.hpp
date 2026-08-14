#pragma once

#include <glm/vec3.hpp>

namespace the_super {

struct Nucleon {
    glm::vec3 position {};
    glm::vec3 velocity {};
    glm::vec3 acceleration {};
    float mass {1.0F};
    bool isProton {true};
    glm::vec3 previousPosition {};
};

} // namespace the_super
