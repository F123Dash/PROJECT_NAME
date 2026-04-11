#include "pipeline.h"
#include "simulator.h"
#include "../network/metrics.h"
#include "../network/queue.h"
#include "../network/logger.h"
#include "../network/failure.h"
#include "../network/congestion.h"
#include <iostream>
#include <stdexcept>

void runSimulationPipeline() {
    Simulator sim;
    
    try {
        // Stage 1: Load configuration
        sim.load_config("config/config.txt");
        
        // Stage 2: Load topology and build graph
        sim.load_topology();
        
        // Stage 3: Initialize system (nodes, routing, layers, metrics)
        sim.init_system();
        
        // Initialize network managers
        Queue::getInstance()->setDefaultCapacity(100);
        Logger::getInstance()->initialize("simulation.log", true);
        FailureManager::getInstance()->enableDebug(false);
        CongestionManager::getInstance()->setBaseDelay(1.0);
        CongestionManager::getInstance()->setCongestionAlpha(0.5);
        
        // Stage 4: Initialize traffic generation
        sim.init_traffic();
        
        // Stage 5: Run simulation
        sim.run();
        
        // Stage 6: Finalize and export results
        sim.finalize();
        
        // Export manager summaries
        Logger::getInstance()->finalize();
        Logger::getInstance()->exportToCSV("logs.csv");
        Queue::getInstance()->printAllQueuesStats();
        FailureManager::getInstance()->printLinkStatus();
        CongestionManager::getInstance()->printCongestionStats();
        MetricsManager::getInstance()->exportMetricsCSV();
        
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        throw;
    }
}
