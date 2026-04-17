#include "simulator.h"
#include "../graphs/graph_generator.h"
#include "event.h"
#include "../network/metrics.h"
#include "../network/logger.h"
#include "../performance/profiler.h"
#include <iostream>
#include <map>
#include <fstream>
#include <sstream>
#include <cmath>

extern double current_time;
extern void run_event_loop(double end_time);
extern void print_metrics();

// Custom stream buffer that writes to both console and file
class TeeStreamBuffer : public std::streambuf {
private:
    std::streambuf* stream1;
    std::streambuf* stream2;
    
public:
    TeeStreamBuffer(std::streambuf* sb1, std::streambuf* sb2) 
        : stream1(sb1), stream2(sb2) {}
    
    int overflow(int c) override {
        if (stream1->sputc(c) == EOF) return EOF;
        return stream2->sputc(c);
    }
    
    int sync() override {
        int r1 = stream1->pubsync();
        int r2 = stream2->pubsync();
        return (r1 == 0 && r2 == 0) ? 0 : -1;
    }
};

static std::ofstream log_file;
static TeeStreamBuffer* tee_buf = nullptr;
static std::streambuf* original_cout_buf = nullptr;

// Helper function to write JSON output
void write_json_output() {
    std::ofstream out("data_output/result.json");
    
    if (!out.is_open()) {
        std::cerr << "[Simulator] ERROR: Could not open data_output/result.json" << std::endl;
        return;
    }
    
    // Get metrics from MetricsManager
    MetricsManager* metrics_mgr = MetricsManager::getInstance();
    GlobalMetrics metrics = metrics_mgr->computeGlobalMetrics();
    
    // Write JSON
    out << "{\n";
    out << "  \"latency\": " << metrics.avg_latency << ",\n";
    out << "  \"throughput\": " << metrics.throughput << ",\n";
    out << "  \"packet_loss\": " << (metrics.loss_rate / 100.0) << ",\n";
    out << "  \"packets_sent\": " << metrics.total_packets << ",\n";
    out << "  \"packets_delivered\": " << metrics.delivered << ",\n";
    out << "  \"packets_dropped\": " << metrics.dropped << ",\n";
    out << "  \"average_hops\": " << metrics.avg_hops << ",\n";
    out << "  \"jitter\": " << metrics.jitter << ",\n";
    out << "  \"min_latency\": " << (metrics.min_latency == INT_MAX ? 0 : metrics.min_latency) << ",\n";
    out << "  \"max_latency\": " << metrics.max_latency << ",\n";
    
    metrics_mgr->exportLatencySeriesToJSON(out);
    
    // Inject the event history directly from the Logger instance natively into the JSON structure
    Logger* logEngine = Logger::getInstance();
    if(logEngine) {
        logEngine->exportToJSON(out);
    } else {
        out << "  \"events\": []\n";
    }
    
    out << "}\n";
    
    out.close();
    std::cout << "[Simulator] JSON output written to data_output/result.json" << std::endl;
}

Simulator::Simulator() {
    graph = nullptr;
    integration = nullptr;
    simulation_time = 1.0;  // Default, will be overridden by config
}

// ---------------- PHASE 0: LOAD CONFIG ----------------
void Simulator::load_config(const std::string& filename) {
    // Set up tee-like output to both console and file
    log_file.open("simulation_console.log", std::ios::out | std::ios::trunc);
    if (log_file.is_open()) {
        original_cout_buf = std::cout.rdbuf();
        tee_buf = new TeeStreamBuffer(original_cout_buf, log_file.rdbuf());
        std::cout.rdbuf(tee_buf);
    }
    
    std::cout << "\n[Simulator] Phase 0: Loading configuration..." << std::endl;
    if (!config.load(filename)) {
        throw std::runtime_error("Failed to load config file: " + filename);
    }
    std::cout << "[Simulator]   Configuration loaded" << std::endl;
}

// Helper: export full adjacency list to JSON for frontend visualization
static void export_topology_json(Graph* graph) {
    std::ofstream topo_out("data_output/topology_edges.json");
    if (!topo_out.is_open()) return;
    topo_out << "[\n";
    bool first = true;
    for (int u = 0; u < graph->V; u++) {
        for (const auto& [v, w] : graph->adj[u]) {
            if (u < v) { // avoid duplicates for undirected
                if (!first) topo_out << ",\n";
                first = false;
                topo_out << "  {\"from\": " << u << ", \"to\": " << v << ", \"weight\": " << w << "}";
            }
        }
    }
    topo_out << "\n]\n";
    topo_out.close();
    std::cout << "[Simulator]   Topology edges exported to data_output/topology_edges.json" << std::endl;
}

// ---------------- PHASE 1: LOAD GRAPH ----------------
void Simulator::load_topology() {
    ScopedTimer timer("load_topology");
    
    std::cout << "\n[Simulator] Phase 1: Loading topology..." << std::endl;
    
    int num_nodes = config.get_int("nodes");
    std::string topology_type = config.get_string("topology");
    
    if (topology_type == "random") {
        double probability = config.get_double("probability");
        graph = new Graph(generate_graph(GraphType::RANDOM, num_nodes, probability, 0, 0, 1, 10));
    } else if (topology_type == "tree") {
        graph = new Graph(generate_graph(GraphType::TREE, num_nodes, 0, 0, 0, 1, 10));
    } else if (topology_type == "grid") {
        int rows = 0, cols = 0;
        try { rows = config.get_int("grid_rows"); } catch(...) {}
        try { cols = config.get_int("grid_cols"); } catch(...) {}
        if (rows <= 0) rows = (int)std::sqrt(num_nodes);
        if (cols <= 0) cols = (num_nodes + rows - 1) / rows;
        graph = new Graph(generate_graph(GraphType::GRID, 0, 0, rows, cols, 1, 10));
    } else {
        throw std::runtime_error("Unknown topology type: " + topology_type);
    }
    
    // Initialize link properties from config
    double bandwidth = config.get_double("bandwidth");
    double latency = config.get_double("delay");
    graph->setDefaultLinkProperties(bandwidth, latency);
    
    // Export full adjacency list for frontend
    export_topology_json(graph);
    
    std::cout << "[Simulator]   Graph created: " << graph->V << " nodes (type=" << topology_type << ")" << std::endl;
    std::cout << "[Simulator]   Link properties: bandwidth=" << bandwidth << " Mbps, latency=" << latency << " ms" << std::endl;
}

// Load topology from custom file (format: u v weight)
void Simulator::load_topology_from_file(const std::string& filename) {
    ScopedTimer timer("load_topology_from_file");
    
    std::cout << "\n[Simulator] Phase 1b: Loading custom topology from file: " << filename << std::endl;
    
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        throw std::runtime_error("Failed to open topology file: " + filename);
    }
    
    int u, v, w;
    int max_node = 0;
    std::vector<std::tuple<int, int, int>> edges;
    
    // Read all edges and find max node ID
    while (infile >> u >> v >> w) {
        edges.push_back({u, v, w});
        max_node = std::max(max_node, u);
        max_node = std::max(max_node, v);
    }
    infile.close();
    
    // Create graph with max_node + 1 nodes
    graph = new Graph(max_node + 1);
    
    // Add edges to graph
    for (const auto& [u, v, w] : edges) {
        graph->add_edge(u, v, w);
    }
    
    // Set link properties from config
    double bandwidth = config.get_double("bandwidth");
    double latency = config.get_double("delay");
    graph->setDefaultLinkProperties(bandwidth, latency);
    
    // Export full adjacency list for frontend
    export_topology_json(graph);
    
    std::cout << "[Simulator]   Graph loaded: " << graph->V << " nodes, " << edges.size() << " edges" << std::endl;
    std::cout << "[Simulator]   Link properties: bandwidth=" << bandwidth << " Mbps, latency=" << latency << " ms" << std::endl;
}

// ---------------- PHASE 2: INIT SYSTEM ----------------
void Simulator::init_system() {
    ScopedTimer timer("init_system");
    
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

    int flow_source = 0;
    int flow_destination = graph->V - 1;
    try {
        flow_source = config.get_int("source");
    } catch (...) {
        // Keep default source when not provided in config.
    }
    try {
        flow_destination = config.get_int("destination");
    } catch (...) {
        // Keep default destination when not provided in config.
    }

    if (flow_source < 0) flow_source = 0;
    if (flow_source >= graph->V) flow_source = graph->V - 1;
    if (flow_destination < 0) flow_destination = 0;
    if (flow_destination >= graph->V) flow_destination = graph->V - 1;

    Flow f;
    f.source = flow_source;
    f.destination = flow_destination;
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
    ScopedTimer timer("simulation_event_loop");
    
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

    // Generate performance profiling report
    Profiler* profiler = Profiler::getInstance();
    if (Profiler::isEnabled() && profiler) {
        std::cout << "\n";
        profiler->report(std::cout);
        profiler->exportCSV("profiler_report.csv");
    }

    // Write JSON result file
    write_json_output();

    std::cout << "\n[Simulator] Done." << std::endl;
    
    // Restore original cout and close log file
    if (tee_buf) {
        std::cout.rdbuf(original_cout_buf);
        delete tee_buf;
        tee_buf = nullptr;
    }
    if (log_file.is_open()) {
        log_file.close();
    }
    
    // Cleanup profiler
    Profiler::destroyInstance();
}