#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "../graphs/graphs.h"
#include "integration.h"
#include "../network/traffic_generator.h"
#include "config.h"
#include <map>

class Simulator {
public:
    Config config;
    Graph* graph;
    Integration* integration;
    TrafficGenerator traffic;
    std::map<int, Node*> node_map;  // Persistent node map with pointers to actual nodes

    double simulation_time;

    Simulator();

    void load_config(const std::string& filename);
    void load_topology();
    void load_topology_from_file(const std::string& filename);
    void init_system();
    void init_traffic();

    void run();
    void finalize();
};

#endif
