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
#include <cmath>
#include <deque>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <queue>
#include <random>
#include <set>
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
    std::string algorithm = "dijkstra";
    std::string traffic = "constant";
    int simulation_time = 15;
    int nodes = 4;
    double probability = 0.5;
    int rows = 2;
    int cols = 2;
    int source = 0;
    int destination = 3;
    int packets = 10;
    int packet_size = 512;
    double rate = 0.0;
    double default_bandwidth = 10.0;
    double default_latency = 1.0;
    int queue_capacity = 64;
    std::vector<int> node_weights;
    std::vector<InputEdge> edges;
};

struct TimedValue {
    double ms = 0.0;
};

static std::string to_lower_copy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

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
        else if (key == "ALGORITHM") in >> req.algorithm;
        else if (key == "TRAFFIC") in >> req.traffic;
        else if (key == "SIMULATION_TIME") in >> req.simulation_time;
        else if (key == "NODES") in >> req.nodes;
        else if (key == "PROBABILITY") in >> req.probability;
        else if (key == "ROWS") in >> req.rows;
        else if (key == "COLS") in >> req.cols;
        else if (key == "SOURCE") in >> req.source;
        else if (key == "DESTINATION") in >> req.destination;
        else if (key == "PACKETS") in >> req.packets;
        else if (key == "PACKET_SIZE") in >> req.packet_size;
        else if (key == "RATE") in >> req.rate;
        else if (key == "DEFAULT_BANDWIDTH") in >> req.default_bandwidth;
        else if (key == "DEFAULT_LATENCY") in >> req.default_latency;
        else if (key == "QUEUE_CAPACITY") in >> req.queue_capacity;
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
    req.simulation_time = std::max(1, req.simulation_time);
    req.packet_size = std::max(1, req.packet_size);
    req.default_bandwidth = std::max(0.001, req.default_bandwidth);
    req.default_latency = std::max(0.0, req.default_latency);
    req.queue_capacity = std::max(1, req.queue_capacity);
    req.source = std::clamp(req.source, 0, req.nodes - 1);
    req.destination = std::clamp(req.destination, 0, req.nodes - 1);
    req.algorithm = to_lower_copy(req.algorithm);
    req.traffic = to_lower_copy(req.traffic);

    if (req.rate <= 0.0) {
        req.rate = (req.packets > 0)
            ? static_cast<double>(req.packets) / static_cast<double>(req.simulation_time)
            : 1.0;
    }
    req.rate = std::max(1e-6, req.rate);

    if (req.algorithm != "dijkstra" && req.algorithm != "mst") {
        req.algorithm = "dijkstra";
    }
    if (req.traffic != "constant" && req.traffic != "bursty" && req.traffic != "poisson") {
        req.traffic = "constant";
    }

    if (req.node_weights.size() < static_cast<std::size_t>(req.nodes)) {
        req.node_weights.resize(req.nodes, 1);
    }

    return req;
}

static int edge_weight(const Graph& graph, int u, int v) {
    if (u < 0 || v < 0 || u >= graph.V || v >= graph.V) return -1;
    for (const auto& [next, weight] : graph.adj[u]) {
        if (next == v) return weight;
    }
    return -1;
}

static int path_cost_from_path(const Graph& graph, const std::vector<int>& path) {
    if (path.empty()) return -1;
    if (path.size() == 1) return 0;

    int total = 0;
    for (std::size_t i = 1; i < path.size(); ++i) {
        int w = edge_weight(graph, path[i - 1], path[i]);
        if (w < 0) return -1;
        total += w;
    }
    return total;
}

static void build_tree_parent_and_dist(
    int n,
    const std::vector<Edge>& tree_edges,
    int source,
    std::vector<int>& dist,
    std::vector<int>& parent
) {
    dist.assign(n, INT_MAX);
    parent.assign(n, -1);
    if (source < 0 || source >= n) return;

    std::vector<std::vector<std::pair<int, int>>> tree_adj(n);
    for (const auto& edge : tree_edges) {
        if (edge.u < 0 || edge.v < 0 || edge.u >= n || edge.v >= n || edge.u == edge.v) continue;
        tree_adj[edge.u].push_back({edge.v, edge.w});
        tree_adj[edge.v].push_back({edge.u, edge.w});
    }

    std::queue<int> q;
    q.push(source);
    dist[source] = 0;

    while (!q.empty()) {
        int u = q.front();
        q.pop();

        for (const auto& [v, w] : tree_adj[u]) {
            if (dist[v] != INT_MAX) continue;
            parent[v] = u;
            dist[v] = dist[u] + std::max(1, w);
            q.push(v);
        }
    }
}

static int to_tick(double time_value) {
    if (!std::isfinite(time_value)) return 0;
    return std::max(0, static_cast<int>(std::llround(time_value)));
}

enum class SimEventType {
    GENERATE_PACKET,
    QUEUE_ENQUEUE,
    QUEUE_DEQUEUE,
    PACKET_SEND,
    PACKET_RECEIVE,
};

static int sim_event_priority(SimEventType type) {
    switch (type) {
        case SimEventType::GENERATE_PACKET: return 0;
        case SimEventType::QUEUE_ENQUEUE: return 1;
        case SimEventType::QUEUE_DEQUEUE: return 2;
        case SimEventType::PACKET_SEND: return 3;
        case SimEventType::PACKET_RECEIVE: return 4;
        default: return 99;
    }
}

struct SimEvent {
    double time = 0.0;
    SimEventType type = SimEventType::GENERATE_PACKET;
    int packet_id = -1;
    int node_id = -1;
    int from_node = -1;
    int to_node = -1;
    int path_index = 0;
    long long seq = 0;
};

struct SimEventComparator {
    bool operator()(const SimEvent& lhs, const SimEvent& rhs) const {
        if (lhs.time != rhs.time) return lhs.time > rhs.time;
        int lhs_priority = sim_event_priority(lhs.type);
        int rhs_priority = sim_event_priority(rhs.type);
        if (lhs_priority != rhs_priority) return lhs_priority > rhs_priority;
        return lhs.seq > rhs.seq;
    }
};

struct QueuedHopPacket {
    int packet_id = -1;
    int path_index = 0;
    double enqueue_time = 0.0;
};

static std::vector<double> build_generation_times(const Request& req, double routing_ready_time) {
    std::vector<double> generation_times;
    generation_times.reserve(std::max(0, req.packets));

    if (req.packets <= 0) return generation_times;

    const double rate = std::max(1e-6, req.rate);
    const double packet_interval = 1.0 / rate;
    double current_time = std::max(0.0, routing_ready_time);

    std::mt19937 rng(42);
    std::uniform_real_distribution<double> unit(1e-9, 1.0 - 1e-9);

    generation_times.push_back(current_time);
    for (int i = 1; i < req.packets; ++i) {
        double interval = packet_interval;

        if (req.traffic == "poisson" || req.traffic == "bursty") {
            // Poisson inter-arrival: interval = -ln(1-u)/lambda
            double lambda = rate;
            if (req.traffic == "bursty") {
                const int burst_size = 4;
                bool in_burst_window = ((i / burst_size) % 2) == 0;
                lambda = in_burst_window ? rate * 2.0 : std::max(1e-6, rate * 0.35);
            }
            const double u = unit(rng);
            interval = -std::log(1.0 - u) / std::max(1e-6, lambda);
        }

        current_time += std::max(1e-6, interval);
        generation_times.push_back(current_time);
    }

    return generation_times;
}

static std::pair<double, double> link_delay_components(
    Graph& graph,
    int from_node,
    int to_node,
    int packet_size_bytes,
    double default_bandwidth,
    double default_latency
) {
    double bandwidth_mbps = std::max(0.001, default_bandwidth);
    double propagation_delay = std::max(0.0, default_latency);

    if (Link* link = graph.getLinkProperties(from_node, to_node)) {
        bandwidth_mbps = std::max(0.001, link->bandwidth);
        propagation_delay = std::max(0.0, link->latency);
    }

    // service_time = packet_size / bandwidth (converted to ms)
    const double transmission_delay =
        (static_cast<double>(std::max(1, packet_size_bytes)) * 8.0) / (bandwidth_mbps * 1000.0);

    // total_delay = propagation_delay + transmission_delay
    const double total_delay = propagation_delay + transmission_delay;
    return {transmission_delay, total_delay};
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
        config.data["algorithm"] = req.algorithm;
        config.data["traffic"] = req.traffic;
        config.data["simulation_time"] = std::to_string(req.simulation_time);
        config.data["packet_count"] = std::to_string(req.packets);
        config.data["rate"] = std::to_string(req.rate);
        config.data["queue_capacity"] = std::to_string(req.queue_capacity);

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

        std::vector<int> selected_dist = dist;
        std::vector<int> selected_parent = parent;
        std::vector<int> selected_path = path;
        int selected_cost = cost;

        if (req.algorithm == "mst") {
            const std::vector<Edge>& selected_tree = (static_cast<int>(prim_tree.size()) == std::max(0, graph->V - 1))
                ? prim_tree
                : kruskal_tree;
            build_tree_parent_and_dist(graph->V, selected_tree, req.source, selected_dist, selected_parent);
            selected_path = build_path(selected_parent, req.source, req.destination);
            selected_cost = path_cost_from_path(*graph, selected_path);
        }

        std::vector<int> selected_routing;
        double selected_routing_ms = measure_ms([&]() {
            selected_routing = buildRoutingTable(selected_parent, selected_dist, req.source, graph->V);
        });

        // Routing is computed before packet generation starts.
        const double routing_ready_time = 0.0;
        std::vector<double> generation_times = build_generation_times(req, routing_ready_time);
        double simulation_end_time = routing_ready_time;

        double simulation_ms = measure_ms([&]() {
            std::priority_queue<SimEvent, std::vector<SimEvent>, SimEventComparator> event_queue;
            std::map<int, std::deque<QueuedHopPacket>> node_queues;
            std::map<int, double> node_next_available_time;
            std::map<int, bool> node_dequeue_scheduled;
            std::set<int> closed_packets;
            long long sequence = 0;

            auto push_event = [&](double time, SimEventType type, int packet_id, int node_id, int from_node, int to_node, int path_index) {
                SimEvent event;
                event.time = std::max(0.0, time);
                event.type = type;
                event.packet_id = packet_id;
                event.node_id = node_id;
                event.from_node = from_node;
                event.to_node = to_node;
                event.path_index = path_index;
                event.seq = sequence++;
                event_queue.push(event);
            };

            for (double generation_time : generation_times) {
                // schedule_event(time + packet_interval, GENERATE_PACKET)
                push_event(generation_time, SimEventType::GENERATE_PACKET, -1, req.source, -1, -1, 0);
            }

            while (!event_queue.empty()) {
                SimEvent event = event_queue.top();
                event_queue.pop();
                simulation_end_time = std::max(simulation_end_time, event.time);

                if (event.packet_id >= 0 && closed_packets.count(event.packet_id) > 0) {
                    continue;
                }

                const int event_tick = to_tick(event.time);

                switch (event.type) {
                    case SimEventType::GENERATE_PACKET: {
                        int packet_id = MetricsManager::getInstance()->onPacketCreated(
                            req.source,
                            req.destination,
                            event_tick,
                            req.packet_size
                        );
                        Logger::getInstance()->logPacketCreated(packet_id, req.source, req.destination, event_tick);

                        if (event.time > req.simulation_time) {
                            MetricsManager::getInstance()->onPacketDropped_TTL(packet_id, event_tick);
                            Logger::getInstance()->logPacketDropped(packet_id, req.source, "SIMULATION_WINDOW_EXCEEDED", event_tick);
                            closed_packets.insert(packet_id);
                            break;
                        }

                        if (selected_path.empty()) {
                            MetricsManager::getInstance()->onPacketDropped_NoRoute(packet_id, event_tick);
                            Logger::getInstance()->logPacketDropped(packet_id, req.source, "NO_ROUTE", event_tick);
                            closed_packets.insert(packet_id);
                            break;
                        }

                        if (selected_path.size() == 1) {
                            Logger::getInstance()->recordHop(packet_id, selected_path.front());
                            MetricsManager::getInstance()->onPacketDelivered(packet_id, event_tick, 0);
                            Logger::getInstance()->logPacketDelivered(packet_id, selected_path.front(), selected_path, event_tick);
                            closed_packets.insert(packet_id);
                            break;
                        }

                        push_event(event.time, SimEventType::QUEUE_ENQUEUE, packet_id, selected_path.front(), -1, -1, 0);
                        break;
                    }

                    case SimEventType::QUEUE_ENQUEUE: {
                        if (event.time > req.simulation_time) {
                            MetricsManager::getInstance()->onPacketDropped_TTL(event.packet_id, event_tick);
                            Logger::getInstance()->logPacketDropped(event.packet_id, event.node_id, "SIMULATION_TIMEOUT", event_tick);
                            closed_packets.insert(event.packet_id);
                            break;
                        }

                        auto& queue_ref = node_queues[event.node_id];
                        if (static_cast<int>(queue_ref.size()) >= req.queue_capacity) {
                            // if(queue.size() >= capacity) drop_packet(); else enqueue(packet);
                            MetricsManager::getInstance()->onPacketDropped_QueueOverflow(event.packet_id, event_tick);
                            Logger::getInstance()->logPacketDropped(event.packet_id, event.node_id, "QUEUE_OVERFLOW", event_tick);
                            closed_packets.insert(event.packet_id);
                            break;
                        }

                        queue_ref.push_back({event.packet_id, event.path_index, event.time});
                        Logger::getInstance()->logQueueEvent(event.packet_id, LogEventType::QUEUE_ENQUEUE, event.node_id, event_tick);

                        if (!node_dequeue_scheduled[event.node_id]) {
                            double dequeue_time = std::max(event.time, node_next_available_time[event.node_id]);
                            push_event(dequeue_time, SimEventType::QUEUE_DEQUEUE, -1, event.node_id, -1, -1, 0);
                            node_dequeue_scheduled[event.node_id] = true;
                        }
                        break;
                    }

                    case SimEventType::QUEUE_DEQUEUE: {
                        node_dequeue_scheduled[event.node_id] = false;
                        auto& queue_ref = node_queues[event.node_id];
                        if (queue_ref.empty()) break;

                        QueuedHopPacket queued = queue_ref.front();
                        queue_ref.pop_front();

                        if (closed_packets.count(queued.packet_id) > 0) {
                            if (!queue_ref.empty() && !node_dequeue_scheduled[event.node_id]) {
                                double next_dequeue = std::max(event.time, node_next_available_time[event.node_id]);
                                push_event(next_dequeue, SimEventType::QUEUE_DEQUEUE, -1, event.node_id, -1, -1, 0);
                                node_dequeue_scheduled[event.node_id] = true;
                            }
                            break;
                        }

                        Logger::getInstance()->logQueueEvent(queued.packet_id, LogEventType::QUEUE_DEQUEUE, event.node_id, event_tick);
                        MetricsManager::getInstance()->onQueueDelay(
                            queued.packet_id,
                            std::max(0, to_tick(event.time - queued.enqueue_time))
                        );

                        if (queued.path_index < 0 || queued.path_index + 1 >= static_cast<int>(selected_path.size())) {
                            MetricsManager::getInstance()->onPacketDropped_NoRoute(queued.packet_id, event_tick);
                            Logger::getInstance()->logPacketDropped(queued.packet_id, event.node_id, "INVALID_PATH", event_tick);
                            closed_packets.insert(queued.packet_id);
                        } else {
                            const int from_node = selected_path[queued.path_index];
                            const int to_node = selected_path[queued.path_index + 1];
                            const auto [service_time, total_delay] = link_delay_components(
                                *graph,
                                from_node,
                                to_node,
                                req.packet_size,
                                req.default_bandwidth,
                                req.default_latency
                            );

                            node_next_available_time[event.node_id] = std::max(node_next_available_time[event.node_id], event.time) + service_time;

                            push_event(event.time, SimEventType::PACKET_SEND, queued.packet_id, from_node, from_node, to_node, queued.path_index);
                            push_event(event.time + total_delay, SimEventType::PACKET_RECEIVE, queued.packet_id, to_node, from_node, to_node, queued.path_index + 1);
                        }

                        if (!queue_ref.empty() && !node_dequeue_scheduled[event.node_id]) {
                            double next_dequeue = std::max(event.time, node_next_available_time[event.node_id]);
                            push_event(next_dequeue, SimEventType::QUEUE_DEQUEUE, -1, event.node_id, -1, -1, 0);
                            node_dequeue_scheduled[event.node_id] = true;
                        }
                        break;
                    }

                    case SimEventType::PACKET_SEND: {
                        if (event.time > req.simulation_time) {
                            MetricsManager::getInstance()->onPacketDropped_TTL(event.packet_id, event_tick);
                            Logger::getInstance()->logPacketDropped(event.packet_id, event.from_node, "SIMULATION_TIMEOUT", event_tick);
                            closed_packets.insert(event.packet_id);
                            break;
                        }

                        Logger::getInstance()->recordHop(event.packet_id, event.from_node);
                        Logger::getInstance()->logRoutingDecision(event.packet_id, event.from_node, req.destination, event.to_node, event_tick);
                        Logger::getInstance()->logPacketForwarded(event.packet_id, event.from_node, event.to_node, event.to_node, event_tick);
                        break;
                    }

                    case SimEventType::PACKET_RECEIVE: {
                        if (event.time > req.simulation_time) {
                            MetricsManager::getInstance()->onPacketDropped_TTL(event.packet_id, event_tick);
                            Logger::getInstance()->logPacketDropped(event.packet_id, event.from_node, "SIMULATION_TIMEOUT", event_tick);
                            closed_packets.insert(event.packet_id);
                            break;
                        }

                        MetricsManager::getInstance()->onPacketHop(event.packet_id, event.to_node, event_tick);

                        const bool reached_destination =
                            event.to_node == req.destination ||
                            event.path_index >= static_cast<int>(selected_path.size()) - 1;

                        if (reached_destination) {
                            Logger::getInstance()->recordHop(event.packet_id, event.to_node);
                            MetricsManager::getInstance()->onPacketDelivered(
                                event.packet_id,
                                event_tick,
                                std::max(0, event.path_index)
                            );
                            Logger::getInstance()->logPacketDelivered(event.packet_id, event.to_node, selected_path, event_tick);
                            closed_packets.insert(event.packet_id);
                        } else {
                            // Keep event ordering strict: enqueue -> transmit -> receive.
                            push_event(event.time, SimEventType::QUEUE_ENQUEUE, event.packet_id, event.to_node, -1, -1, event.path_index);
                        }
                        break;
                    }
                }
            }
        });

        MetricsManager::getInstance()->finalize(
            std::max(req.simulation_time, static_cast<int>(std::ceil(simulation_end_time)))
        );
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
        json << "\"algorithm\":\"" << json_escape(req.algorithm) << "\",";
        json << "\"traffic\":\"" << json_escape(req.traffic) << "\",";
        json << "\"simulationTime\":" << req.simulation_time << ",";
        json << "\"path\":[";
        for (std::size_t i = 0; i < selected_path.size(); ++i) {
            if (i) json << ",";
            json << selected_path[i];
        }
        json << "],";
        json << "\"pathCost\":" << selected_cost << ",";
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
        json << "\"routingTableMs\":" << selected_routing_ms << ",";
        json << "\"kruskalMs\":" << kruskal_ms << ",";
        json << "\"primMs\":" << prim_ms << ",";
        json << "\"packetSimulationMs\":" << simulation_ms;
        json << "},";
        json << "\"routingTable\":[";
        for (std::size_t i = 0; i < selected_routing.size(); ++i) {
            if (i) json << ",";
            json << selected_routing[i];
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
