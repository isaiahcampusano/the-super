#include "the_super/Nucleus.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <random>

#include <glm/geometric.hpp>

namespace the_super {

namespace {

constexpr float minimumDistance = 0.20F;
constexpr float forceSoftening = 0.35F;
constexpr float maximumStep = 1.0F / 120.0F;
constexpr float maximumFrameTime = 0.10F;
constexpr float fusionInitialDistance = 8.0F;
constexpr float fusionTriggerDistance = 1.5F;
constexpr float fissionTriggerSeconds = 3.0F;

struct SeedNucleon {
    glm::vec3 position;
    glm::vec3 velocity;
    bool isProton;
};

constexpr std::array<SeedNucleon, 7> sevenNucleonSeed {{
    {{0.0F, 0.0F, 0.0F}, {0.00F, 0.02F, 0.00F}, true},
    {{0.9F, 0.0F, 0.0F}, {0.00F, 0.12F, 0.02F}, false},
    {{-0.9F, 0.0F, 0.0F}, {0.00F, -0.12F, -0.02F}, false},
    {{0.0F, 0.9F, 0.0F}, {-0.10F, 0.00F, 0.03F}, true},
    {{0.0F, -0.9F, 0.0F}, {0.10F, 0.00F, -0.03F}, true},
    {{0.0F, 0.0F, 0.9F}, {0.04F, -0.03F, 0.00F}, false},
    {{0.0F, 0.0F, -0.9F}, {-0.04F, 0.03F, 0.00F}, true},
}};

glm::vec3 centroid(const std::vector<Nucleon>& nucleons, const std::vector<int>& groups, int group) {
    glm::vec3 center {0.0F};
    int count = 0;
    for (std::size_t index = 0; index < nucleons.size(); ++index) {
        if (groups[index] == group) {
            center += nucleons[index].position;
            ++count;
        }
    }
    return count > 0 ? center / static_cast<float>(count) : center;
}

} // namespace

Nucleus::Nucleus() {
    initialize(Scenario::Stable);
}

void Nucleus::initialize(Scenario scenario) {
    currentScenario_ = scenario;
    status_ = SimulationStatus::Idle;
    triggerOccurred_ = false;
    triggerProgress_ = 0.0F;
    elapsedTime_ = 0.0F;

    switch (scenario) {
    case Scenario::Stable:
        setupStableNucleus();
        break;
    case Scenario::Fusion:
        setupFusionNuclei();
        break;
    case Scenario::Fission:
        setupFissionNucleus();
        break;
    }
}

void Nucleus::setupStableNucleus(int count) {
    nucleons_.clear();
    groupIds_.clear();
    const int safeCount = std::clamp(count, 1, static_cast<int>(sevenNucleonSeed.size()));
    nucleons_.reserve(static_cast<std::size_t>(safeCount));
    groupIds_.reserve(static_cast<std::size_t>(safeCount));
    for (int index = 0; index < safeCount; ++index) {
        const SeedNucleon& seed = sevenNucleonSeed[static_cast<std::size_t>(index)];
        nucleons_.push_back({
            seed.position,
            seed.velocity,
            {},
            1.0F,
            seed.isProton,
            seed.position,
        });
        groupIds_.push_back(0);
    }
}

void Nucleus::setupFusionNuclei(int groupSize) {
    nucleons_.clear();
    groupIds_.clear();
    const int safeGroupSize = std::clamp(
        groupSize,
        1,
        static_cast<int>(sevenNucleonSeed.size())
    );
    nucleons_.reserve(static_cast<std::size_t>(safeGroupSize * 2));
    groupIds_.reserve(static_cast<std::size_t>(safeGroupSize * 2));

    for (int group = 0; group < 2; ++group) {
        const float side = group == 0 ? -1.0F : 1.0F;
        const glm::vec3 center {side * 4.0F, 0.0F, 0.0F};
        const glm::vec3 approachVelocity {-side * 0.5F, 0.0F, 0.0F};
        for (int index = 0; index < safeGroupSize; ++index) {
            const SeedNucleon& seed = sevenNucleonSeed[static_cast<std::size_t>(index)];
            const glm::vec3 position = center + seed.position;
            nucleons_.push_back({
                position,
                approachVelocity + (seed.velocity * 0.35F),
                {},
                1.0F,
                seed.isProton,
                position,
            });
            groupIds_.push_back(group);
        }
    }
}

void Nucleus::setupFissionNucleus(int totalCount) {
    nucleons_.clear();
    groupIds_.clear();
    const int safeCount = std::max(totalCount, 1);
    nucleons_.reserve(static_cast<std::size_t>(safeCount));
    groupIds_.reserve(static_cast<std::size_t>(safeCount));

    std::mt19937 generator(0xF15510U);
    std::uniform_real_distribution<float> coordinate(-2.0F, 2.0F);
    std::uniform_real_distribution<float> jitter(-0.025F, 0.025F);
    for (int index = 0; index < safeCount; ++index) {
        glm::vec3 position;
        do {
            position = {coordinate(generator), coordinate(generator), coordinate(generator)};
        } while (glm::dot(position, position) > 4.0F);

        const bool isProton = index < std::min(20, safeCount);
        const glm::vec3 velocity {jitter(generator), jitter(generator), jitter(generator)};
        nucleons_.push_back({position, velocity, {}, 1.0F, isProton, position});
        groupIds_.push_back(position.x > 0.0F ? 1 : 0);
    }
}

void Nucleus::start() {
    status_ = SimulationStatus::Running;
}

void Nucleus::pause() {
    if (status_ != SimulationStatus::Idle) {
        status_ = SimulationStatus::Paused;
    }
}

void Nucleus::reset() {
    initialize(currentScenario_);
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

void Nucleus::updatePhysics(float deltaTime) {
    if (status_ == SimulationStatus::Idle || status_ == SimulationStatus::Paused) {
        return;
    }

    float remainingTime = std::clamp(deltaTime, 0.0F, maximumFrameTime);
    while (remainingTime > 0.0F) {
        const float step = std::min(remainingTime, maximumStep);
        integrateStep(step);
        elapsedTime_ += step;

        if (currentScenario_ == Scenario::Fission && !triggerOccurred_) {
            triggerProgress_ = std::clamp(elapsedTime_ / fissionTriggerSeconds, 0.0F, 1.0F);
            if (elapsedTime_ >= fissionTriggerSeconds) {
                executeFissionSplit();
            }
        }
        remainingTime -= step;
    }
}

void Nucleus::checkFusionCondition() {
    if (triggerOccurred_ || groupIds_.size() != nucleons_.size()) {
        return;
    }
    const glm::vec3 firstCenter = centroid(nucleons_, groupIds_, 0);
    const glm::vec3 secondCenter = centroid(nucleons_, groupIds_, 1);
    const float distance = glm::length(secondCenter - firstCenter);
    triggerProgress_ = std::clamp(
        (fusionInitialDistance - distance) / (fusionInitialDistance - fusionTriggerDistance),
        0.0F,
        1.0F
    );
    if (distance >= fusionTriggerDistance) {
        return;
    }

    std::fill(groupIds_.begin(), groupIds_.end(), 0);
    for (Nucleon& nucleon : nucleons_) {
        nucleon.velocity *= 0.5F;
    }
    triggerOccurred_ = true;
    triggerProgress_ = 1.0F;
    status_ = SimulationStatus::Triggered;
}

void Nucleus::executeFissionSplit() {
    glm::vec3 center {0.0F};
    for (const Nucleon& nucleon : nucleons_) {
        center += nucleon.position;
    }
    if (!nucleons_.empty()) {
        center /= static_cast<float>(nucleons_.size());
    }

    std::array<glm::vec3, 2> meanVelocity {};
    std::array<int, 2> groupCounts {};
    std::vector<std::size_t> xOrder(nucleons_.size());
    std::iota(xOrder.begin(), xOrder.end(), std::size_t {0});
    std::sort(
        xOrder.begin(),
        xOrder.end(),
        [this](std::size_t left, std::size_t right) {
            return nucleons_[left].position.x < nucleons_[right].position.x;
        }
    );
    for (std::size_t order = 0; order < xOrder.size(); ++order) {
        const std::size_t index = xOrder[order];
        const int group = order < xOrder.size() / 2U ? 0 : 1;
        groupIds_[index] = group;
        meanVelocity[static_cast<std::size_t>(group)] += nucleons_[index].velocity;
        ++groupCounts[static_cast<std::size_t>(group)];
    }
    for (std::size_t group = 0; group < meanVelocity.size(); ++group) {
        if (groupCounts[group] > 0) {
            meanVelocity[group] /= static_cast<float>(groupCounts[group]);
        }
    }

    for (std::size_t index = 0; index < nucleons_.size(); ++index) {
        Nucleon& nucleon = nucleons_[index];
        const int group = groupIds_[index];
        const float side = group == 1 ? 1.0F : -1.0F;
        glm::vec3 radial = nucleon.position - center;
        if (glm::length(radial) <= 1.0e-5F) {
            radial = glm::vec3 {side, 0.0F, 0.0F};
        } else {
            radial = glm::normalize(radial);
        }
        const glm::vec3 internalVelocity = nucleon.velocity
            - meanVelocity[static_cast<std::size_t>(group)];
        nucleon.velocity = glm::vec3 {side * 1.60F, 0.0F, 0.0F}
            + (internalVelocity * 0.08F)
            + (radial * 0.12F);
    }

    triggerOccurred_ = true;
    triggerProgress_ = 1.0F;
    status_ = SimulationStatus::Triggered;
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
            const bool acrossFissionFragments = currentScenario_ == Scenario::Fission
                && triggerOccurred_
                && groupIds_[first] != groupIds_[second];
            const float strongMagnitude = acrossFissionFragments
                ? 0.0F
                : parameters_.strongForce
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

    if (currentScenario_ == Scenario::Fusion) {
        checkFusionCondition();
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
