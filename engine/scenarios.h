#pragma once

#include <vector>

enum class ScenarioType {
    LOW_TRAFFIC,
    HIGH_CONGESTION,
    FAILURE
};

struct ScenarioResult {
    double avg_latency;
    double avg_throughput;
    double avg_loss;
};

class ScenarioRunner {
public:
    static ScenarioResult runScenario(ScenarioType type, int runs = 3);
};
