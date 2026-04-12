#include "pipeline.h"
#include <iostream>
#include <stdexcept>

Pipeline::Pipeline(Simulator* s) {
    if (!s) {
        throw std::runtime_error("Pipeline: Simulator pointer is NULL");
    }
    sim = s;
}

void Pipeline::execute() {
    std::cout << "\n===== EXECUTION PIPELINE START =====\n" << std::endl;

    try {
        // ============================================================
        // STEP 1: LOAD CONFIG
        // ============================================================
        std::cout << "[1/7] Loading configuration..." << std::endl;
        sim->load_config("config/config.txt");
        std::cout << "         Configuration loaded\n" << std::endl;

        // ============================================================
        // STEP 2: BUILD GRAPH
        // ============================================================
        std::cout << "[2/7] Building graph..." << std::endl;
        sim->load_topology();
        if (!sim->graph) {
            throw std::runtime_error("Graph initialization failed");
        }
        std::cout << "         Graph created (" << sim->graph->V << " nodes)\n" << std::endl;

        // ============================================================
        // STEP 3: INITIALIZE SYSTEM (Nodes + Network Layers)
        // ============================================================
        std::cout << "[3/7] Initializing system..." << std::endl;
        sim->init_system();
        if (!sim->integration) {
            throw std::runtime_error("Integration object not initialized");
        }
        std::cout << "         System initialized\n" << std::endl;

        // ============================================================
        // STEP 4: VERIFY ROUTING TABLES
        // ============================================================
        std::cout << "[4/7] Verifying routing tables..." << std::endl;
        if (sim->integration->nodes.empty()) {
            throw std::runtime_error("No nodes initialized");
        }
        for (size_t i = 0; i < sim->integration->nodes.size(); i++) {
            if (sim->integration->nodes[i]->routing_table.empty()) {
                throw std::runtime_error("Node " + std::to_string(i) + " has no routing table");
            }
        }
        std::cout << "         All routing tables verified\n" << std::endl;

        // ============================================================
        // STEP 5: INITIALIZE TRAFFIC
        // ============================================================
        std::cout << "[5/7] Initializing traffic..." << std::endl;
        sim->init_traffic();
        std::cout << "         Traffic flows scheduled\n" << std::endl;

        // ============================================================
        // STEP 6: RUN SIMULATION
        // ============================================================
        std::cout << "[6/7] Running simulation..." << std::endl;
        sim->run();
        std::cout << "         Simulation complete\n" << std::endl;

        // ============================================================
        // STEP 7: FINALIZE & COLLECT METRICS
        // ============================================================
        std::cout << "[7/7] Collecting and finalizing metrics..." << std::endl;
        sim->finalize();
        std::cout << "         Metrics finalized\n" << std::endl;

        std::cout << "\n===== EXECUTION PIPELINE SUCCESS =====" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "\n[PIPELINE ERROR] " << e.what() << std::endl;
        std::cerr << "===== EXECUTION PIPELINE FAILED =====" << std::endl;
        throw;
    }
}