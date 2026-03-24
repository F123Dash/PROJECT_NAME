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

// Get neighbors of u
std::vector<std::pair<int,int>> Graph::get_neighbors(int u) const {
    validate_node(u);
    return adj[u];
}