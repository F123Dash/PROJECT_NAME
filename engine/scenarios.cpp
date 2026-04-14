#include "scenarios.h"
#include "../engine/simulator.h"
#include "../engine/pipeline.h"
#include "../engine/data_collector.h"
#include "../engine/statistics.h"

ScenarioResult ScenarioRunner::runScenario(ScenarioType type, int runs) {

    DataCollector data_collector("temp_metrics.csv", "temp_log.txt");

    for (int i = 1; i <= runs; i++) {

        Simulator sim;
        Pipeline pipeline(&sim, &data_collector);

        // 🔥 Scenario-specific configuration
        switch (type) {
            case ScenarioType::LOW_TRAFFIC:
                sim.set_packet_rate(10);  // low load
                break;

            case ScenarioType::HIGH_CONGESTION:
                sim.set_packet_rate(1000); // heavy load
                sim.set_queue_limit(10);   // small queue → congestion
                break;

            case ScenarioType::FAILURE:
                sim.enable_failures(true); // simulate failures
                break;
        }

        pipeline.execute();
    }

    const auto& latencies = data_collector.get_latencies();
    const auto& throughputs = data_collector.get_throughputs();
    const auto& losses = data_collector.get_loss_rates();

    ScenarioResult result;

    result.avg_latency = Statistics::average(latencies);
    result.avg_throughput = Statistics::average(throughputs);
    result.avg_loss = Statistics::average(losses);

    return result;
}
