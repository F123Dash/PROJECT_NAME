#ifndef TRAFFIC_GENERATOR_H
#define TRAFFIC_GENERATOR_H

#include "flow.h"
#include "../src/packet.h"
#include "../src/event.h"
#include <vector>
#include <map>

// Forward declarations
struct Node;

class TrafficGenerator {
public:
    TrafficGenerator() = default;

    // Add a single flow
    void add_flow(const Flow& f);

    // Schedule all flows (call once at simulation start)
    void schedule_flows(std::map<int, Node>& nodes_map);

private:
    std::vector<Flow> flows;
    std::map<int, Node>* node_map_ptr;

    // Generate packet for a flow at a specific time
    void generate_packet(const Flow& f, double time, std::map<int, Node>& nodes_map);

    // Schedule next packet for a flow
    void schedule_next_packet(const Flow& f, double next_time, std::map<int, Node>& nodes_map);
};

#endif
