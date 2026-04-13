#include "../engine/simulator.h"
#include "../engine/pipeline.h"
#include "../engine/data_collector.h"
#include <iostream>
#include <stdexcept>

int main() {
    try {
        // Initialize data collection
        DataCollector data_collector("data_metrics.csv", "data_collection.log");
        
        Simulator sim;
        Pipeline pipeline(&sim, &data_collector);

        pipeline.execute();

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[MAIN] Fatal error: " << e.what() << std::endl;
        return 1;
    }
}