#include "../engine/integration.h"
#include "../graphs/graph_generator.h"
#include "../network/traffic_generator.h"
#include <iostream>
using namespace std;

int main() {
    cout << "=== Network Simulator Integration Test ===" << endl;

    // Step 1: Generate network topology
    cout << "\n[1] Generating network topology..." << endl;
    Graph* graph = new Graph(generate_graph(GraphType::RANDOM, 5, 0.6));  // 5 nodes, 60% edge probability
    if (!graph) {
        cerr << "Failed to generate graph!" << endl;
        return 1;
    }
    cout << "    Graph created: " << graph->V << " nodes" << endl;

    // Step 2: Create Integration system
    cout << "\n[2] Creating Integration system..." << endl;
    Integration* integration = new Integration(graph);
    GLOBAL_INTEGRATION = integration;
    cout << "    Integration object created and set as global" << endl;

    // Step 3: Initialize nodes
    cout << "\n[3] Initializing nodes..." << endl;
    integration->init_nodes();
    cout << "    Created " << integration->nodes.size() << " nodes" << endl;

    // Step 4: Build routing tables using Dijkstra
    cout << "\n[4] Building routing tables (Dijkstra)..." << endl;
    integration->build_routing_tables();
    cout << "    Routing tables built for all nodes" << endl;

    // Step 5: Attach network and transport layers
    cout << "\n[5] Attaching network and transport layers..." << endl;
    bool use_tcp = true;  // Use TCP (set to false for UDP)
    integration->attach_layers(use_tcp);
    cout << "    Layers attached (" << (use_tcp ? "TCP" : "UDP") << ")" << endl;

    // Step 6: Verify connectivity
    cout << "\n[6] Verifying system connectivity..." << endl;
    integration->connect_nodes();
    cout << "    Connectivity verified" << endl;

    // Step 7: Print node information
    cout << "\n[7] Node status:" << endl;
    for (int i = 0; i < integration->nodes.size(); i++) {
        Node* node = integration->get_node(i);
        if (node) {
            cout << "    Node " << i << ": routes=" << node->routing_table.size() 
                 << ", interfaces=" << node->interfaces.size() << endl;
            
            // Print first few routes
            cout << "      Routes: ";
            int num_routes_to_show = min(5, (int)node->routing_table.size());
            for (int j = 0; j < num_routes_to_show; j++) {
                if (node->routing_table[j] != -1) {
                    cout << j << "->" << node->routing_table[j] << " ";
                }
            }
            cout << endl;
        }
    }

    // Step 8: Cleanup
    cout << "\n[8] Cleaning up..." << endl;
    delete integration;
    delete graph;
    GLOBAL_INTEGRATION = nullptr;
    cout << "    Resources freed" << endl;

    cout << "\n=== Integration Test Complete ===" << endl;
    return 0;
}
