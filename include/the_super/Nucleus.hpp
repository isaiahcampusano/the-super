#pragma once

#include <vector>

#include "the_super/Nucleon.hpp"
#include "the_super/SimulationState.hpp"

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

    void initialize(Scenario scenario);
    void updatePhysics(float deltaTime);
    void start();
    void pause();
    void reset();

    void setPhysicsParameters(const PhysicsParameters& parameters);
    [[nodiscard]] const PhysicsParameters& physicsParameters() const;
    [[nodiscard]] const std::vector<Nucleon>& getNucleons() const;
    [[nodiscard]] SimulationStatus getStatus() const { return status_; }
    [[nodiscard]] Scenario getScenario() const { return currentScenario_; }
    [[nodiscard]] bool hasTriggered() const { return triggerOccurred_; }
    [[nodiscard]] float getTriggerProgress() const { return triggerProgress_; }

private:
    void setupStableNucleus(int count = 7);
    void setupFusionNuclei(int groupSize = 7);
    void setupFissionNucleus(int totalCount = 30);
    void checkFusionCondition();
    void executeFissionSplit();
    void integrateStep(float deltaTime);

    std::vector<Nucleon> nucleons_;
    std::vector<int> groupIds_;
    PhysicsParameters parameters_;
    SimulationStatus status_ {SimulationStatus::Idle};
    Scenario currentScenario_ {Scenario::Stable};
    bool triggerOccurred_ {false};
    float triggerProgress_ {};
    float elapsedTime_ {};
};

} // namespace the_super
