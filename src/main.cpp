#include "../engine/simulator.h"
#include "../engine/pipeline.h"
#include "../engine/data_collector.h"
#include "../engine/statistics.h"
#include "../network/metrics_types.h"
#include <iostream>
#include <stdexcept>

int main() {
    try {
        // Initialize data collection
        int runs = 5;
        DataCollector data_collector("data_metrics.csv", "data_collection.log");
        for(int i = 1;i<=runs;i++)
        {
            std::cout << "\n===== RUN " << i << " =====\n";
            Simulator sim;
            Pipeline pipeline(&sim, &data_collector);

            pipeline.execute();
        }
        

        const std::vector<double>& latencies = data_collector.get_latencies();
        const std::vector<double>& throughputs = data_collector.get_throughputs();
        const std::vector<double>& losses = data_collector.get_loss_rates();

        std::cout << "\n===== FINAL STATISTICS =====\n";

        std::cout << "\n--- LATENCY ---\n";
        std::cout << "Avg: " << Statistics::average(latencies) << std::endl;
        std::cout << "Var: " << Statistics::variance(latencies) << std::endl;
        std::cout << "Min: " << Statistics::minimum(latencies) << std::endl;
        std::cout << "Max: " << Statistics::maximum(latencies) << std::endl;

        std::cout << "\n--- THROUGHPUT ---\n";
        std::cout << "Avg: " << Statistics::average(throughputs) << std::endl;
        std::cout << "Var: " << Statistics::variance(throughputs) << std::endl;
        std::cout << "Min: " << Statistics::minimum(throughputs) << std::endl;
        std::cout << "Max: " << Statistics::maximum(throughputs) << std::endl;

        std::cout << "\n--- LOSS ---\n";
        std::cout << "Avg: " << Statistics::average(losses) << std::endl;
        std::cout << "Var: " << Statistics::variance(losses) << std::endl;
        std::cout << "Min: " << Statistics::minimum(losses) << std::endl;
        std::cout << "Max: " << Statistics::maximum(losses) << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[MAIN] Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
