#include "simulator.h"
#include "../graphs/graph_generator.h"
#include "event.h"
#include "../network/metrics.h"
#include "../network/logger.h"
#include <iostream>
#include <map>

extern double current_time;
extern void run_event_loop(double end_time);
extern void print_metrics();

Simulator::Simulator() {
    graph = nullptr;
    integration = nullptr;
    simulation_time = 1.0;  // Default, will be overridden by config
}

// ---------------- PHASE 0: LOAD CONFIG ----------------
void Simulator::load_config(const std::string& filename) {
    std::cout << "\n[Simulator] Phase 0: Loading configuration..." << std::endl;
    if (!config.load(filename)) {
        throw std::runtime_error("Failed to load config file: " + filename);
    }
    std::cout << "[Simulator]   Configuration loaded" << std::endl;
}

// ---------------- PHASE 1: LOAD GRAPH ----------------
void Simulator::load_topology() {
    std::cout << "\n[Simulator] Phase 1: Loading topology..." << std::endl;
    
    int num_nodes = config.get_int("nodes");
    std::string topology_type = config.get_string("topology");
    
    if (topology_type == "random") {
        double probability = config.get_double("probability");
        graph = new Graph(generate_graph(GraphType::RANDOM, num_nodes, probability, 0, 0, 1, 10));
    } else if (topology_type == "tree") {
        graph = new Graph(generate_graph(GraphType::TREE, num_nodes, 0, 0, 0, 1, 10));
    } else {
        throw std::runtime_error("Unknown topology type: " + topology_type);
    }
    
    // Initialize link properties from config
    double bandwidth = config.get_double("bandwidth");
    double latency = config.get_double("delay");
    graph->setDefaultLinkProperties(bandwidth, latency);
    
    std::cout << "[Simulator]   Graph created: " << graph->V << " nodes (type=" << topology_type << ")" << std::endl;
    std::cout << "[Simulator]   Link properties: bandwidth=" << bandwidth << " Mbps, latency=" << latency << " ms" << std::endl;
}

// ---------------- PHASE 2: INIT SYSTEM ----------------
void Simulator::init_system() {
    std::cout << "\n[Simulator] Phase 2: Initializing system..." << std::endl;

    // Set simulation time from config
    simulation_time = config.get_double("simulation_time");

    // ============ LOAD REAL-TIME CONTROL SETTINGS ============
    extern bool REAL_TIME_MODE;
    extern double TIME_SCALE;
    extern int MAX_SLEEP_MS;

    REAL_TIME_MODE = config.get_bool("real_time_mode");
    TIME_SCALE = config.get_double("time_scale");
    MAX_SLEEP_MS = config.get_int("max_sleep_ms");

    if (REAL_TIME_MODE) {
        std::cout << "[Simulator]   Real-time mode ENABLED (time_scale=" << TIME_SCALE << "x)" << std::endl;
    } else {
        std::cout << "[Simulator]   Real-time mode disabled (max speed)" << std::endl;
    }
    // ===========================================================

    // ============ LOAD RETRANSMISSION SETTINGS ================
    extern bool ENABLE_RETRANSMISSION;
    extern double RTX_TIMEOUT_MS;
    extern int MAX_RETRANSMISSIONS;

    ENABLE_RETRANSMISSION = config.get_bool("enable_retransmission");
    RTX_TIMEOUT_MS = config.get_double("rtx_timeout_ms");
    MAX_RETRANSMISSIONS = config.get_int("max_retransmissions");

    if (ENABLE_RETRANSMISSION) {
        std::cout << "[Simulator]   Retransmission ENABLED (timeout=" << RTX_TIMEOUT_MS 
                  << "ms, max_retries=" << MAX_RETRANSMISSIONS << ")" << std::endl;
    } else {
        std::cout << "[Simulator]   Retransmission disabled" << std::endl;
    }
    // ===========================================================

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
    
    // Initialize logging system
    Logger::getInstance()->initialize("simulation_events.log", true);
    std::cout << "[Simulator]   Event logging initialized" << std::endl;
}

// ---------------- PHASE 3: INIT TRAFFIC ----------------
void Simulator::init_traffic() {
    std::cout << "\n[Simulator] Phase 3: Initializing traffic..." << std::endl;

    Flow f;
    f.source = 0;
    f.destination = graph->V - 1;
    f.rate = config.get_double("rate");
    f.packet_size = config.get_int("packet_size");
    f.start_time = 0;
    f.duration = simulation_time;
    
    std::string traffic_type_str = config.get_string("traffic_type");
    if (traffic_type_str == "constant") {
        f.type = TrafficType::CONSTANT;
    } else if (traffic_type_str == "random") {
        f.type = TrafficType::RANDOM;
    } else if (traffic_type_str == "burst") {
        f.type = TrafficType::BURST;
    } else {
        throw std::runtime_error("Unknown traffic type: " + traffic_type_str);
    }

    traffic.add_flow(f);
    std::cout << "[Simulator]   Flow added: " << f.source << " → " << f.destination 
              << " (rate=" << f.rate << " pkt/s, type=" << traffic_type_str << ")" << std::endl;

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
    
    // Finalize and export logs
    Logger* logger = Logger::getInstance();
    if (logger) {
        logger->finalize();
        // Export event logs for analysis
        logger->exportToFile();
        logger->exportToCSV("simulation_events.csv");
        std::cout << "[Simulator]   Event logs exported" << std::endl;
    }
    
    std::cout << "\n--- Metrics ---" << std::endl;

    print_metrics();

    std::cout << "\n[Simulator] Done." << std::endl;
}
