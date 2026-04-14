#include "../engine/scenarios.h"
#include <iostream>

void printResult(const std::string& name, const ScenarioResult& res) {
    std::cout << "\n===== " << name << " =====\n";
    std::cout << "Avg Latency   : " << res.avg_latency << "\n";
    std::cout << "Avg Throughput: " << res.avg_throughput << "\n";
    std::cout << "Avg Loss Rate : " << res.avg_loss << "\n";
}

int main() {
    try {
        auto low = ScenarioRunner::runScenario(ScenarioType::LOW_TRAFFIC, 3);
        auto high = ScenarioRunner::runScenario(ScenarioType::HIGH_CONGESTION, 3);
        auto failure = ScenarioRunner::runScenario(ScenarioType::FAILURE, 3);

        std::cout << "\n========== FINAL COMPARISON ==========\n";

        printResult("LOW TRAFFIC", low);
        printResult("HIGH CONGESTION", high);
        printResult("FAILURE", failure);

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "[MAIN] Error: " << e.what() << std::endl;
        return 1;
    }
}
