#include "the_super/Nucleus.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

#include <glm/geometric.hpp>

namespace the_super {

namespace {

constexpr float minimumDistance = 0.20F;
constexpr float forceSoftening = 0.35F;
constexpr float maximumStep = 1.0F / 120.0F;
constexpr float maximumFrameTime = 0.10F;

std::vector<Nucleon> defaultNucleons() {
    return {
        {{0.0F, 0.0F, 0.0F}, {0.00F, 0.02F, 0.00F}, {}, 1.0F, true},
        {{0.9F, 0.0F, 0.0F}, {0.00F, 0.12F, 0.02F}, {}, 1.0F, false},
        {{-0.9F, 0.0F, 0.0F}, {0.00F, -0.12F, -0.02F}, {}, 1.0F, false},
        {{0.0F, 0.9F, 0.0F}, {-0.10F, 0.00F, 0.03F}, {}, 1.0F, true},
        {{0.0F, -0.9F, 0.0F}, {0.10F, 0.00F, -0.03F}, {}, 1.0F, true},
        {{0.0F, 0.0F, 0.9F}, {0.04F, -0.03F, 0.00F}, {}, 1.0F, false},
        {{0.0F, 0.0F, -0.9F}, {-0.04F, 0.03F, 0.00F}, {}, 1.0F, true},
    };
}

} // namespace

Nucleus::Nucleus()
    : nucleons_(defaultNucleons()) {
    for (Nucleon& nucleon : nucleons_) {
        nucleon.previousPosition = nucleon.position;
    }
}

Nucleus::Nucleus(std::vector<Nucleon> nucleons)
    : nucleons_(std::move(nucleons)) {
    for (Nucleon& nucleon : nucleons_) {
        nucleon.previousPosition = nucleon.position;
    }
}

void Nucleus::setPhysicsParameters(const PhysicsParameters& parameters) {
    parameters_ = parameters;
    parameters_.strongForce = std::max(parameters_.strongForce, 0.0F);
    parameters_.strongRange = std::max(parameters_.strongRange, minimumDistance);
    parameters_.coulombForce = std::max(parameters_.coulombForce, 0.0F);
    parameters_.damping = std::clamp(parameters_.damping, 0.0F, 1.0F);
    parameters_.boundaryRadius = std::max(parameters_.boundaryRadius, minimumDistance);
}

const PhysicsParameters& Nucleus::physicsParameters() const {
    return parameters_;
}

const std::vector<Nucleon>& Nucleus::getNucleons() const {
    return nucleons_;
}

void Nucleus::reset() {
    nucleons_ = defaultNucleons();
    for (Nucleon& nucleon : nucleons_) {
        nucleon.previousPosition = nucleon.position;
    }
}

void Nucleus::updatePhysics(float deltaTime) {
    float remainingTime = std::clamp(deltaTime, 0.0F, maximumFrameTime);
    while (remainingTime > 0.0F) {
        const float step = std::min(remainingTime, maximumStep);
        integrateStep(step);
        remainingTime -= step;
    }
}

void Nucleus::integrateStep(float deltaTime) {
    for (Nucleon& nucleon : nucleons_) {
        nucleon.acceleration = glm::vec3 {0.0F};
    }

    for (std::size_t first = 0; first < nucleons_.size(); ++first) {
        for (std::size_t second = first + 1; second < nucleons_.size(); ++second) {
            Nucleon& a = nucleons_[first];
            Nucleon& b = nucleons_[second];
            const glm::vec3 displacement = b.position - a.position;
            const float rawDistance = glm::length(displacement);
            if (rawDistance <= 1.0e-6F) {
                continue;
            }

            const float distanceSquared = (rawDistance * rawDistance)
                + (forceSoftening * forceSoftening);
            const float distance = std::sqrt(distanceSquared);
            const glm::vec3 direction = displacement / distance;
            const float rangeRatio = distance / parameters_.strongRange;
            const float strongMagnitude = parameters_.strongForce
                * parameters_.strongForce
                * std::exp(-rangeRatio)
                * (1.0F + rangeRatio)
                / distanceSquared;
            const float chargeProduct = a.isProton && b.isProton ? 1.0F : 0.0F;
            const float coulombMagnitude = parameters_.coulombForce
                * chargeProduct
                / distanceSquared;

            const glm::vec3 forceOnA = (strongMagnitude - coulombMagnitude) * direction;
            a.acceleration += forceOnA / std::max(a.mass, 1.0e-6F);
            b.acceleration -= forceOnA / std::max(b.mass, 1.0e-6F);
        }
    }

    const float stepDamping = std::pow(parameters_.damping, deltaTime * 60.0F);
    for (Nucleon& nucleon : nucleons_) {
        nucleon.previousPosition = nucleon.position;
        nucleon.velocity += nucleon.acceleration * deltaTime;
        nucleon.velocity *= stepDamping;
        nucleon.position += nucleon.velocity * deltaTime;

        const float radius = glm::length(nucleon.position);
        if (radius > parameters_.boundaryRadius) {
            const glm::vec3 outward = nucleon.position / radius;
            nucleon.position = outward * parameters_.boundaryRadius;
            const float outwardSpeed = glm::dot(nucleon.velocity, outward);
            if (outwardSpeed > 0.0F) {
                nucleon.velocity -= outward * outwardSpeed;
            }
        }
    }
}

} // namespace the_super
