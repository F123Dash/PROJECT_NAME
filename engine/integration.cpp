#include "integration.h"
#include "../algorithms/shortest_path.h"
#include <iostream>
#include <climits>

using namespace std;

// Global integration pointer (initialized in main)
Integration* GLOBAL_INTEGRATION = nullptr;

Integration::Integration(Graph* g) {
    graph = g;
}

Integration::~Integration() {
    // Clean up nodes
    for (auto node : nodes) {
        if (node->network_layer) delete node->network_layer;
        if (node->transport) delete node->transport;
        delete node;
    }
    nodes.clear();
}

// Step 1: Create Nodes
void Integration::init_nodes() {
    int n = graph->V;
    nodes.resize(n);

    cout << "[Integration] Creating " << n << " nodes..." << endl;

    for (int i = 0; i < n; i++) {
        nodes[i] = new Node(i);
        nodes[i]->graph = graph;
    }

    cout << "[Integration]   " << n << " nodes created" << endl;
}

// Step 2: Build Routing Tables using Dijkstra from shortest_path.h
void Integration::build_routing_tables() {
    int n = graph->V;

    cout << "[Integration] Building routing tables using Dijkstra..." << endl;

    for (int i = 0; i < n; i++) {
        vector<int> dist(n, INT_MAX);
        vector<int> parent(n, -1);

        // Run Dijkstra from node i
        dijkstra(*graph, i, dist, parent);

        // Build routing table: for each destination, store next hop
        vector<int> routing(n, -1);
        for (int dest = 0; dest < n; dest++) {
            if (dest == i) {
                routing[dest] = i;  // routing to self is self
                continue;
            }

            if (dist[dest] != INT_MAX) {
                // Trace parent chain to find next hop
                int current = dest;
                while (parent[current] != i && parent[current] != -1) {
                    current = parent[current];
                }
                routing[dest] = current;
            } else {
                routing[dest] = -1;  // unreachable
            }
        }

        nodes[i]->routing_table = routing;
    }

    cout << "[Integration]   Routing tables built" << endl;
}

// Step 3: Attach Network + Transport
void Integration::attach_layers(bool use_tcp) {
    cout << "[Integration] Attaching Network and Transport layers..." << endl;

    for (auto node : nodes) {
        // Network layer
        node->network_layer = new NetworkLayer(node);
        cout << "  [Node " << node->id << "] NetworkLayer attached" << endl;

        // Transport layer (choose TCP or UDP)
        if (use_tcp) {
            node->transport = new TCP();
        } else {
            node->transport = new UDP();
        }
    }

    cout << "[Integration]   All layers attached" << endl;
}

Node* Integration::get_node(int id) {
    if (id < 0 || id >= static_cast<int>(nodes.size())) {
        cerr << "[Integration] ERROR: Invalid node ID " << id << endl;
        return nullptr;
    }
    return nodes[id];
}

void Integration::connect_nodes() {
    cout << "[Integration] Verifying node connectivity..." << endl;

    for (auto node : nodes) {
        if (!node->network_layer) {
            cerr << "[Integration] ERROR: Node " << node->id << " has no network layer!" << endl;
            continue;
        }
        if (!node->transport) {
            cerr << "[Integration] ERROR: Node " << node->id << " has no transport!" << endl;
            continue;
        }

        // Check routing table
        bool has_routes = false;
        for (int dest = 0; dest < static_cast<int>(node->routing_table.size()); dest++) {
            if (node->routing_table[dest] != -1) {
                has_routes = true;
                break;
            }
        }

        if (!has_routes) {
            cout << "  [Node " << node->id << "] WARNING: No routes found" << endl;
        }
    }

    cout << "[Integration]   Connectivity verified" << endl;
}
