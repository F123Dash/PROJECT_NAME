#include "../algorithms/mst.h"
#include "../algorithms/routing_table.h"
#include "../algorithms/shortest_path.h"
#include "../engine/config.h"
#include "../engine/integration.h"
#include "../engine/statistics.h"
#include "../graphs/graph_analysis.h"
#include "../graphs/graph_generator.h"
#include "../network/debug.h"
#include "../network/logger.h"
#include "../network/metrics.h"

#include <algorithm>
#include <chrono>
#include <climits>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

struct InputEdge {
    int u = 0;
    int v = 0;
    int w = 1;
    double bandwidth = 10.0;
    double latency = 1.0;
};

struct Request {
    std::string topology = "custom";
    int nodes = 4;
    double probability = 0.5;
    int rows = 2;
    int cols = 2;
    int source = 0;
    int destination = 3;
    int packets = 10;
    int packet_size = 512;
    double default_bandwidth = 10.0;
    double default_latency = 1.0;
    std::vector<int> node_weights;
    std::vector<InputEdge> edges;
};

struct TimedValue {
    double ms = 0.0;
};

static std::string json_escape(const std::string& value) {
    std::ostringstream out;
    for (char c : value) {
        switch (c) {
            case '\\': out << "\\\\"; break;
            case '"': out << "\\\""; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default: out << c; break;
        }
    }
    return out.str();
}

static std::string read_file_tail(const std::string& path, std::size_t max_chars = 12000) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string text = buffer.str();
    if (text.size() > max_chars) {
        return text.substr(text.size() - max_chars);
    }
    return text;
}

static Request parse_request(std::istream& in) {
    Request req;
    std::string key;

    while (in >> key) {
        if (key == "TOPOLOGY") in >> req.topology;
        else if (key == "NODES") in >> req.nodes;
        else if (key == "PROBABILITY") in >> req.probability;
        else if (key == "ROWS") in >> req.rows;
        else if (key == "COLS") in >> req.cols;
        else if (key == "SOURCE") in >> req.source;
        else if (key == "DESTINATION") in >> req.destination;
        else if (key == "PACKETS") in >> req.packets;
        else if (key == "PACKET_SIZE") in >> req.packet_size;
        else if (key == "DEFAULT_BANDWIDTH") in >> req.default_bandwidth;
        else if (key == "DEFAULT_LATENCY") in >> req.default_latency;
        else if (key == "NODE_WEIGHTS") {
            int count = 0;
            in >> count;
            req.node_weights.resize(std::max(0, count));
            for (int& weight : req.node_weights) in >> weight;
        } else if (key == "EDGES") {
            int count = 0;
            in >> count;
            req.edges.resize(std::max(0, count));
            for (InputEdge& edge : req.edges) {
                in >> edge.u >> edge.v >> edge.w >> edge.bandwidth >> edge.latency;
            }
        } else if (key == "END") {
            break;
        }
    }

    req.nodes = std::max(1, req.nodes);
    req.packets = std::max(0, req.packets);
    req.source = std::clamp(req.source, 0, req.nodes - 1);
    req.destination = std::clamp(req.destination, 0, req.nodes - 1);
    if (req.node_weights.size() < static_cast<std::size_t>(req.nodes)) {
        req.node_weights.resize(req.nodes, 1);
    }

    return req;
}

static std::unique_ptr<Graph> build_graph(const Request& req) {
    if (req.topology == "random") {
        return std::make_unique<Graph>(
            generate_graph(GraphType::RANDOM, req.nodes, req.probability, 0, 0, 1, 10)
        );
    }

    if (req.topology == "tree") {
        return std::make_unique<Graph>(
            generate_graph(GraphType::TREE, req.nodes, 0.0, 0, 0, 1, 10)
        );
    }

    if (req.topology == "grid") {
        return std::make_unique<Graph>(
            generate_graph(GraphType::GRID, 0, 0.0, req.rows, req.cols, 1, 10)
        );
    }

    auto graph = std::make_unique<Graph>(req.nodes);
    for (const InputEdge& edge : req.edges) {
        if (edge.u >= 0 && edge.u < req.nodes && edge.v >= 0 && edge.v < req.nodes && edge.u != edge.v) {
            graph->add_edge(edge.u, edge.v, std::max(1, edge.w));
            graph->setLinkProperties(edge.u, edge.v, edge.bandwidth, edge.latency);
        }
    }
    return graph;
}

static int edge_count(const Graph& graph) {
    int total = 0;
    for (const auto& neighbors : graph.adj) total += static_cast<int>(neighbors.size());
    return total / 2;
}

static std::vector<InputEdge> graph_edges(const Graph& graph, double bandwidth, double latency) {
    std::vector<InputEdge> edges;
    for (int u = 0; u < graph.V; ++u) {
        for (const auto& [v, w] : graph.adj[u]) {
            if (u < v) edges.push_back({u, v, w, bandwidth, latency});
        }
    }
    return edges;
}

static int path_cost(const Graph& graph, const std::vector<int>& parent, int source, int destination) {
    if (source == destination) return 0;
    if (destination < 0 || destination >= graph.V || parent[destination] == -1) return -1;

    int total = 0;
    int current = destination;
    while (current != source) {
        int previous = parent[current];
        if (previous == -1) return -1;
        bool found = false;
        for (const auto& [next, weight] : graph.adj[previous]) {
            if (next == current) {
                total += weight;
                found = true;
                break;
            }
        }
        if (!found) return -1;
        current = previous;
    }
    return total;
}

static std::vector<int> build_path(const std::vector<int>& parent, int source, int destination) {
    std::vector<int> path;
    if (source == destination) return {source};
    if (destination < 0 || parent[destination] == -1) return path;

    for (int current = destination; current != -1; current = parent[current]) {
        path.push_back(current);
        if (current == source) break;
    }
    std::reverse(path.begin(), path.end());
    if (path.empty() || path.front() != source) path.clear();
    return path;
}

template <typename Fn>
static double measure_ms(Fn fn) {
    auto start = std::chrono::high_resolution_clock::now();
    fn();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

int main() {
    const std::string debug_path = "backend/debug.log";
    const std::string events_path = "backend/simulation_events.log";
    const std::string events_csv_path = "backend/simulation_events.csv";
    const std::string metrics_path = "backend/metrics.csv";

    std::streambuf* original_cout = std::cout.rdbuf();
    std::ofstream debug_file(debug_path, std::ios::out | std::ios::trunc);
    if (debug_file.is_open()) std::cout.rdbuf(debug_file.rdbuf());

    try {
        Request req = parse_request(std::cin);
        std::unique_ptr<Graph> graph = build_graph(req);
        graph->setDefaultLinkProperties(req.default_bandwidth, req.default_latency);

        if (req.topology != "custom") {
            req.nodes = graph->V;
            req.source = std::clamp(req.source, 0, req.nodes - 1);
            req.destination = std::clamp(req.destination, 0, req.nodes - 1);
            req.edges = graph_edges(*graph, req.default_bandwidth, req.default_latency);
            req.node_weights.resize(req.nodes, 1);
        }

        Config config;
        config.data["nodes"] = std::to_string(req.nodes);
        config.data["topology"] = req.topology;
        config.data["packet_count"] = std::to_string(req.packets);

        Logger::getInstance()->initialize(events_path, true);
        MetricsManager::getInstance()->initialize(0, metrics_path);

        std::unique_ptr<Integration> integration = std::make_unique<Integration>(graph.get());
        GLOBAL_INTEGRATION = integration.get();

        double init_ms = measure_ms([&]() {
            integration->init_nodes();
            integration->build_routing_tables();
            integration->attach_layers(false);
            integration->connect_nodes();
        });

        std::vector<int> dist;
        std::vector<int> parent;
        double dijkstra_ms = measure_ms([&]() {
            dijkstra(*graph, req.source, dist, parent);
        });

        std::vector<int> bellman_dist;
        std::vector<int> bellman_parent;
        bool bellman_ok = true;
        double bellman_ms = measure_ms([&]() {
            bellman_ok = bellmanFord(*graph, req.source, bellman_dist, bellman_parent);
        });

        std::vector<int> routing;
        double routing_ms = measure_ms([&]() {
            routing = buildRoutingTable(parent, dist, req.source, graph->V);
        });

        std::vector<Edge> edges_for_kruskal;
        for (int u = 0; u < graph->V; ++u) {
            for (const auto& [v, w] : graph->adj[u]) {
                if (u < v) edges_for_kruskal.push_back({u, v, w});
            }
        }

        std::vector<Edge> kruskal_tree;
        double kruskal_ms = measure_ms([&]() {
            kruskal_tree = kruskal(graph->V, edges_for_kruskal);
        });

        std::vector<Edge> prim_tree;
        double prim_ms = measure_ms([&]() {
            prim_tree = prim(*graph);
        });

        std::vector<int> path = build_path(parent, req.source, req.destination);
        int cost = path_cost(*graph, parent, req.source, req.destination);

        double simulation_ms = measure_ms([&]() {
            for (int i = 0; i < req.packets; ++i) {
                int packet_id = MetricsManager::getInstance()->onPacketCreated(
                    req.source, req.destination, i, req.packet_size
                );
                if (path.empty()) {
                    MetricsManager::getInstance()->onPacketDropped_NoRoute(packet_id, i);
                    Logger::getInstance()->logPacketDropped(packet_id, req.source, "NO_ROUTE", i);
                    continue;
                }
                for (int node_id : path) {
                    MetricsManager::getInstance()->onPacketHop(packet_id, node_id, i);
                    Logger::getInstance()->recordHop(packet_id, node_id);
                }
                MetricsManager::getInstance()->onPacketDelivered(packet_id, i + std::max(0, cost), static_cast<int>(path.size()));
                Logger::getInstance()->logPacketDelivered(packet_id, req.destination, path, i + std::max(0, cost));
            }
        });

        MetricsManager::getInstance()->finalize(std::max(1, req.packets + std::max(0, cost)));
        MetricsManager::getInstance()->exportMetricsToFile(metrics_path);
        Logger::getInstance()->finalize();
        Logger::getInstance()->exportToCSV(events_csv_path);

        GlobalMetrics metrics = MetricsManager::getInstance()->computeGlobalMetrics();
        std::vector<double> algorithm_times = {dijkstra_ms, bellman_ms, routing_ms, kruskal_ms, prim_ms};

        std::cout.rdbuf(original_cout);

        std::ostringstream json;
        json << std::fixed << std::setprecision(4);
        json << "{";
        json << "\"ok\":true,";
        json << "\"topology\":\"" << json_escape(req.topology) << "\",";
        json << "\"nodes\":" << graph->V << ",";
        json << "\"edges\":" << edge_count(*graph) << ",";
        json << "\"averageDegree\":" << average_degree(*graph) << ",";
        json << "\"source\":" << req.source << ",";
        json << "\"destination\":" << req.destination << ",";
        json << "\"path\":[";
        for (std::size_t i = 0; i < path.size(); ++i) {
            if (i) json << ",";
            json << path[i];
        }
        json << "],";
        json << "\"pathCost\":" << cost << ",";
        json << "\"bellmanFordOk\":" << (bellman_ok ? "true" : "false") << ",";
        json << "\"metrics\":{";
        json << "\"totalPackets\":" << metrics.total_packets << ",";
        json << "\"delivered\":" << metrics.delivered << ",";
        json << "\"dropped\":" << metrics.dropped << ",";
        json << "\"droppedTtl\":" << metrics.dropped_ttl << ",";
        json << "\"droppedNoRoute\":" << metrics.dropped_no_route << ",";
        json << "\"averageLatency\":" << metrics.avg_latency << ",";
        json << "\"minLatency\":" << (metrics.min_latency == INT_MAX ? 0 : metrics.min_latency) << ",";
        json << "\"maxLatency\":" << metrics.max_latency << ",";
        json << "\"jitter\":" << metrics.jitter << ",";
        json << "\"lossRate\":" << metrics.loss_rate << ",";
        json << "\"averageHops\":" << metrics.avg_hops << ",";
        json << "\"throughput\":" << metrics.throughput;
        json << "},";
        json << "\"statistics\":{";
        json << "\"algorithmAverageMs\":" << Statistics::average(algorithm_times) << ",";
        json << "\"algorithmVarianceMs\":" << Statistics::variance(algorithm_times) << ",";
        json << "\"algorithmMinMs\":" << Statistics::minimum(algorithm_times) << ",";
        json << "\"algorithmMaxMs\":" << Statistics::maximum(algorithm_times);
        json << "},";
        json << "\"timings\":{";
        json << "\"integrationMs\":" << init_ms << ",";
        json << "\"dijkstraMs\":" << dijkstra_ms << ",";
        json << "\"bellmanFordMs\":" << bellman_ms << ",";
        json << "\"routingTableMs\":" << routing_ms << ",";
        json << "\"kruskalMs\":" << kruskal_ms << ",";
        json << "\"primMs\":" << prim_ms << ",";
        json << "\"packetSimulationMs\":" << simulation_ms;
        json << "},";
        json << "\"routingTable\":[";
        for (std::size_t i = 0; i < routing.size(); ++i) {
            if (i) json << ",";
            json << routing[i];
        }
        json << "],";
        json << "\"graphEdges\":[";
        std::vector<InputEdge> response_edges = graph_edges(*graph, req.default_bandwidth, req.default_latency);
        for (std::size_t i = 0; i < response_edges.size(); ++i) {
            if (i) json << ",";
            json << "{\"u\":" << response_edges[i].u
                 << ",\"v\":" << response_edges[i].v
                 << ",\"w\":" << response_edges[i].w << "}";
        }
        json << "],";
        json << "\"files\":{";
        json << "\"debug\":\"" << debug_path << "\",";
        json << "\"events\":\"" << events_path << "\",";
        json << "\"eventsCsv\":\"" << events_csv_path << "\",";
        json << "\"metricsCsv\":\"" << metrics_path << "\"";
        json << "},";
        json << "\"debugTail\":\"" << json_escape(read_file_tail(debug_path)) << "\"";
        json << "}";

        std::cout << json.str() << std::endl;

        MetricsManager::destroyInstance();
        Logger::destroyInstance();
        GLOBAL_INTEGRATION = nullptr;
        return 0;
    } catch (const std::exception& ex) {
        std::cout.rdbuf(original_cout);
        std::cout << "{\"ok\":false,\"error\":\"" << json_escape(ex.what()) << "\"}" << std::endl;
        return 1;
    }
}
