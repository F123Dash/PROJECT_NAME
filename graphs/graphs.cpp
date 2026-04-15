#include "graphs.h"

// Constructor
Graph::Graph(int n) {
    if (n <= 0) {
        throw std::invalid_argument("Number of vertices must be positive");
    }
    V = n;
    adj.resize(V);
}

// Validate node index
void Graph::validate_node(int u) const {
    if (u < 0 || u >= V) {
        throw std::out_of_range("Invalid node index");
    }
}

// Add edge (undirected)
void Graph::add_edge(int u, int v, int w) {
    validate_node(u);
    validate_node(v);

    for (auto &p : adj[u]) {
        if (p.first == v) return;
    }

    adj[u].push_back({v, w});
    adj[v].push_back({u, w});
}

// Remove edge
void Graph::remove_edge(int u, int v) {
    validate_node(u);
    validate_node(v);

    // Remove v from u's list
    for (auto it = adj[u].begin(); it != adj[u].end(); ++it) {
        if (it->first == v) {
            adj[u].erase(it);
            break;
        }
    }

    // Remove u from v's list
    for (auto it = adj[v].begin(); it != adj[v].end(); ++it) {
        if (it->first == u) {
            adj[v].erase(it);
            break;
        }
    }
}

// Get neighbors of u (const reference - preferred to avoid copies)
const std::vector<std::pair<int,int>>& Graph::get_neighbors(int u) const {
    validate_node(u);
    return adj[u];
}

// Legacy copy method (deprecated - use const reference version)
std::vector<std::pair<int,int>> Graph::get_neighbors_copy(int u) const {
    validate_node(u);
    return adj[u];
}

// Set link properties (bandwidth in Mbps, latency in ms)
void Graph::setLinkProperties(int u, int v, double bandwidth, double latency) {
    validate_node(u);
    validate_node(v);
    
    links[{u, v}] = Link(u, v, bandwidth, latency);
    links[{v, u}] = Link(v, u, bandwidth, latency);
}

// Get link properties (returns nullptr if not found)
Link* Graph::getLinkProperties(int u, int v) {
    validate_node(u);
    validate_node(v);
    
    auto it = links.find({u, v});
    if (it != links.end()) {
        return &(it->second);
    }
    return nullptr;
}

// Set default link properties for all edges
void Graph::setDefaultLinkProperties(double bandwidth, double latency) {
    for (int u = 0; u < V; u++) {
        for (auto& edge : adj[u]) {
            int v = edge.first;
            if (u < v) {  // Only set once for undirected edges
                setLinkProperties(u, v, bandwidth, latency);
            }
        }
    }
}