#include "../engine/simulator.h"
#include "../engine/pipeline.h"
#include "../engine/data_collector.h"
#include "../engine/statistics.h"
#include "../network/metrics_types.h"
#include "../performance/profiler.h"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <iomanip>

// ============================================================================
// JSON Output Helper
// ============================================================================
void printJSON(const std::vector<double>& latencies,
               const std::vector<double>& throughputs,
               const std::vector<double>& losses) {
    
    std::cout << "{\n";
    std::cout << "  \"ok\": true,\n";
    std::cout << "  \"statistics\": {\n";
    std::cout << "    \"latency\": {\n";
    std::cout << "      \"avg\": " << std::fixed << std::setprecision(4) 
              << Statistics::average(latencies) << ",\n";
    std::cout << "      \"var\": " << Statistics::variance(latencies) << ",\n";
    std::cout << "      \"min\": " << Statistics::minimum(latencies) << ",\n";
    std::cout << "      \"max\": " << Statistics::maximum(latencies) << "\n";
    std::cout << "    },\n";
    
    std::cout << "    \"throughput\": {\n";
    std::cout << "      \"avg\": " << Statistics::average(throughputs) << ",\n";
    std::cout << "      \"var\": " << Statistics::variance(throughputs) << ",\n";
    std::cout << "      \"min\": " << Statistics::minimum(throughputs) << ",\n";
    std::cout << "      \"max\": " << Statistics::maximum(throughputs) << "\n";
    std::cout << "    },\n";
    
    std::cout << "    \"loss\": {\n";
    std::cout << "      \"avg\": " << Statistics::average(losses) << ",\n";
    std::cout << "      \"var\": " << Statistics::variance(losses) << ",\n";
    std::cout << "      \"min\": " << Statistics::minimum(losses) << ",\n";
    std::cout << "      \"max\": " << Statistics::maximum(losses) << "\n";
    std::cout << "    }\n";
    std::cout << "  },\n";
    std::cout << "  \"runs\": " << latencies.size() << "\n";
    std::cout << "}\n";
}

// ============================================================================
// MAIN
// ============================================================================
int main(int argc, char* argv[]) {
    try {
        std::string config_file = "config/config.txt";
        std::string topo_file = "";

        // Parse arguments: simulator [config_file] [topology_file]
        if (argc > 1) {
            config_file = argv[1];
        }
        if (argc > 2) {
            topo_file = argv[2];
        }

        Simulator sim;
        sim.load_config(config_file);
        if (!topo_file.empty()) {
            sim.load_topology_from_file(topo_file);
        } else {
            sim.load_topology();
        }
        
        sim.init_system();
        sim.init_traffic();
        sim.run();
        sim.finalize();

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[MAIN] Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
