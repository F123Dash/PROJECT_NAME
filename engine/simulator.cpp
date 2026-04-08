#include "simulator.h"
#include "../graphs/graph_generator.h"
#include "event.h"
#include "../network/metrics.h"
#include <iostream>
#include <map>

extern double current_time;
extern void run_event_loop(double end_time);
extern void print_metrics();

Simulator::Simulator() {
    graph = nullptr;
    integration = nullptr;
    simulation_time = 1.0;  // Reduced for testing
}

// ---------------- PHASE 1: LOAD GRAPH ----------------
void Simulator::load_topology() {
    std::cout << "\n[Simulator] Phase 1: Loading topology..." << std::endl;
    graph = new Graph(generate_graph(GraphType::TREE, 5, 0, 0, 0, 1, 10));  // Tree topology (guaranteed connected)
    std::cout << "[Simulator]   Graph created: " << graph->V << " nodes" << std::endl;
}

// ---------------- PHASE 2: INIT SYSTEM ----------------
void Simulator::init_system() {
    std::cout << "\n[Simulator] Phase 2: Initializing system..." << std::endl;

    integration = new Integration(graph);
    GLOBAL_INTEGRATION = integration;
    std::cout << "[Simulator]   Integration object created" << std::endl;

    integration->init_nodes();
    std::cout << "[Simulator]   Nodes initialized" << std::endl;

    integration->build_routing_tables();
    std::cout << "[Simulator]   Routing tables built" << std::endl;

    integration->attach_layers(true);  // Use TCP
    std::cout << "[Simulator]   Network and transport layers attached" << std::endl;

    // Initialize metrics tracking
    MetricsManager::getInstance()->initialize(0, "");
    std::cout << "[Simulator]   Metrics engine initialized" << std::endl;
}

// ---------------- PHASE 3: INIT TRAFFIC ----------------
void Simulator::init_traffic() {
    std::cout << "\n[Simulator] Phase 3: Initializing traffic..." << std::endl;

    Flow f;
    f.source = 0;
    f.destination = graph->V - 1;
    f.rate = 5;
    f.packet_size = 512;
    f.start_time = 0;
    f.duration = simulation_time;
    f.type = TrafficType::CONSTANT;

    traffic.add_flow(f);
    std::cout << "[Simulator]   Flow added: " << f.source << " → " << f.destination << std::endl;

    // Build persistent node map with pointers to actual nodes
    for (int i = 0; i < integration->nodes.size(); i++) {
        if (integration->nodes[i] != nullptr) {
            node_map[i] = integration->nodes[i];  // Store pointer, not copy
        }
    }

    traffic.schedule_flows(node_map);
    std::cout << "[Simulator]   Traffic scheduled" << std::endl;
}

// ---------------- PHASE 4: RUN ----------------
void Simulator::run() {
    std::cout << "\n[Simulator] Phase 4: Running simulation (t=0 to t=" << simulation_time << ")..." << std::endl;
    run_event_loop(simulation_time);
    std::cout << "[Simulator]   Simulation finished" << std::endl;
}

// ---------------- PHASE 5: FINALIZE ----------------
void Simulator::finalize() {
    std::cout << "\n[Simulator] Phase 5: Finalizing simulation..." << std::endl;
    
    // Finalize metrics
    MetricsManager::getInstance()->finalize((int)simulation_time);
    
    std::cout << "\n--- Metrics ---" << std::endl;

    print_metrics();

    std::cout << "\n[Simulator] Done." << std::endl;
}
