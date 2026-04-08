#ifndef INTEGRATION_H
#define INTEGRATION_H

#include "../graphs/graphs.h"
#include "../algorithms/routing_table.h"
#include "../network/network_layer.h"
#include "../network/tcp.h"
#include "../network/udp.h"
#include "node.h"

#include <vector>
#include <memory>

class Integration {
public:
    Graph* graph;
    std::vector<Node*> nodes;

    explicit Integration(Graph* g);
    ~Integration();

    // Step 1: Create all nodes
    void init_nodes();

    // Step 2: Build routing tables using Dijkstra
    void build_routing_tables();

    // Step 3: Attach Network Layer and Transport Layer to each node
    void attach_layers(bool use_tcp = false);

    // Global access to nodes
    Node* get_node(int id);
    
    // Utility
    int num_nodes() const { return nodes.size(); }
    void connect_nodes();
};

// Global integration reference (set in main.cpp)
extern Integration* GLOBAL_INTEGRATION;

#endif
