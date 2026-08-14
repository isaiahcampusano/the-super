#pragma once

#include <vector>

#include "the_super/Nucleon.hpp"

namespace the_super {

struct PhysicsParameters {
    float strongForce {1.0F};
    float strongRange {1.5F};
    float coulombForce {0.5F};
    float damping {0.999F};
    float boundaryRadius {5.0F};
};

class Nucleus {
public:
    Nucleus();
    explicit Nucleus(std::vector<Nucleon> nucleons);

    void updatePhysics(float deltaTime);
    void reset();

    void setPhysicsParameters(const PhysicsParameters& parameters);
    [[nodiscard]] const PhysicsParameters& physicsParameters() const;
    [[nodiscard]] const std::vector<Nucleon>& getNucleons() const;

private:
    void integrateStep(float deltaTime);

    std::vector<Nucleon> nucleons_;
    PhysicsParameters parameters_;
};

} // namespace the_super
