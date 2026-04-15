#include "pipeline.h"
#include "../network/metrics.h"
#include "../performance/profiler.h"
#include <iostream>
#include <stdexcept>

Pipeline::Pipeline(Simulator* s, DataCollector* dc) 
    : data_collector(dc) {
    if (!s) {
        throw std::runtime_error("Pipeline: Simulator pointer is NULL");
    }
    sim = s;
}

void Pipeline::execute() {
    std::cout << "\n===== EXECUTION PIPELINE START =====\n" << std::endl;
    
    if (data_collector) {
        data_collector->start_run(1);
        data_collector->log_step("[1/7]", "Pipeline execution started");
    }

    try {
        // ============================================================
        // STEP 1: LOAD CONFIG
        // ============================================================
        std::cout << "[1/7] Loading configuration..." << std::endl;
        if (data_collector) data_collector->log_step("[1/7]", "Loading configuration");
        
        sim->load_config("config/config.txt");
        std::cout << "         Configuration loaded\n" << std::endl;
        if (data_collector) data_collector->log_step("[1/7]", "Configuration loaded successfully");
        
        // Enable profiling based on config
        if (sim->config.get_bool("enable_profiling")) {
            Profiler::enable();
            std::cout << "[PROFILER] Performance profiling ENABLED\n" << std::endl;
        } else {
            Profiler::disable();
            std::cout << "[PROFILER] Performance profiling disabled\n" << std::endl;
        }

        // ============================================================
        // STEP 2: BUILD GRAPH
        // ============================================================
        std::cout << "[2/7] Building graph..." << std::endl;
        if (data_collector) data_collector->log_step("[2/7]", "Building network topology");
        
        sim->load_topology();
        if (!sim->graph) {
            throw std::runtime_error("Graph initialization failed");
        }
        std::cout << "         Graph created (" << sim->graph->V << " nodes)\n" << std::endl;
        if (data_collector) data_collector->log_step("[2/7]", 
            "Graph created with " + std::to_string(sim->graph->V) + " nodes");

        // ============================================================
        // STEP 3: INITIALIZE SYSTEM (Nodes + Network Layers)
        // ============================================================
        std::cout << "[3/7] Initializing system..." << std::endl;
        if (data_collector) data_collector->log_step("[3/7]", "Initializing system nodes and layers");
        
        sim->init_system();
        if (!sim->integration) {
            throw std::runtime_error("Integration object not initialized");
        }
        std::cout << "         System initialized\n" << std::endl;
        if (data_collector) data_collector->log_step("[3/7]", "System initialization complete");

        // ============================================================
        // STEP 4: VERIFY ROUTING TABLES
        // ============================================================
        std::cout << "[4/7] Verifying routing tables..." << std::endl;
        if (data_collector) data_collector->log_step("[4/7]", "Verifying routing tables");
        
        if (sim->integration->nodes.empty()) {
            throw std::runtime_error("No nodes initialized");
        }
        for (size_t i = 0; i < sim->integration->nodes.size(); i++) {
            if (sim->integration->nodes[i]->routing_table.empty()) {
                throw std::runtime_error("Node " + std::to_string(i) + " has no routing table");
            }
        }
        std::cout << "         All routing tables verified\n" << std::endl;
        if (data_collector) data_collector->log_step("[4/7]", 
            "Verified routing tables for " + std::to_string(sim->integration->nodes.size()) + " nodes");

        // ============================================================
        // STEP 5: INITIALIZE TRAFFIC
        // ============================================================
        std::cout << "[5/7] Initializing traffic..." << std::endl;
        if (data_collector) data_collector->log_step("[5/7]", "Initializing traffic flows");
        
        sim->init_traffic();
        std::cout << "         Traffic flows scheduled\n" << std::endl;
        if (data_collector) data_collector->log_step("[5/7]", "Traffic flows initialized");

        // ============================================================
        // STEP 6: RUN SIMULATION
        // ============================================================
        std::cout << "[6/7] Running simulation..." << std::endl;
        if (data_collector) data_collector->log_step("[6/7]", "Starting simulation execution");
        
        sim->run();
        std::cout << "         Simulation complete\n" << std::endl;
        if (data_collector) data_collector->log_step("[6/7]", "Simulation execution completed");

        // ============================================================
        // STEP 7: FINALIZE & COLLECT METRICS
        // ============================================================
        std::cout << "[7/7] Collecting and finalizing metrics..." << std::endl;
        if (data_collector) data_collector->log_step("[7/7]", "Collecting metrics and finalizing");
        
        sim->finalize();
        std::cout << "         Metrics finalized\n" << std::endl;
        
        // Retrieve and record metrics
        if (data_collector) {
            MetricsManager* metrics_mgr = MetricsManager::getInstance();
            if (metrics_mgr) {
                GlobalMetrics global_metrics = metrics_mgr->computeGlobalMetrics();
                data_collector->record_metrics(global_metrics);
                data_collector->log_step("[7/7]", "Metrics recorded successfully");
            }
        }

        std::cout << "\n===== EXECUTION PIPELINE SUCCESS =====" << std::endl;
        if (data_collector) data_collector->log_event("Pipeline execution completed successfully");

    } catch (const std::exception& e) {
        std::cerr << "\n[PIPELINE ERROR] " << e.what() << std::endl;
        std::cerr << "===== EXECUTION PIPELINE FAILED =====" << std::endl;
        if (data_collector) {
            data_collector->log_error("Pipeline execution failed: " + std::string(e.what()));
        }
        throw;
    }
}