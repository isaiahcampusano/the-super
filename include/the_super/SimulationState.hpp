#pragma once

namespace the_super {

enum class Scenario {
    Stable,
    Fusion,
    Fission,
};

enum class SimulationStatus {
    Idle,
    Running,
    Paused,
    Triggered,
};

} // namespace the_super
