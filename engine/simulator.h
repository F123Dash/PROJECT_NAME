#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "../graphs/graphs.h"
#include "integration.h"
#include "../network/traffic_generator.h"
#include <map>

class Simulator {
public:
    Graph* graph;
    Integration* integration;
    TrafficGenerator traffic;
    std::map<int, Node*> node_map;  // Persistent node map with pointers to actual nodes

    double simulation_time;

    Simulator();

    void load_topology();
    void init_system();
    void init_traffic();

    void run();
    void finalize();
};

#endif
