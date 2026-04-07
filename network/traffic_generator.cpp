#include "traffic_generator.h"
#include "../engine/node.h"
#include <random>
#include <iostream>

// Global event scheduler (must be provided by simulation engine)
extern std::vector<Event> event_queue;
extern double current_simulation_time;

void schedule_event(const Event& e) {
    event_queue.push_back(e);
    std::push_heap(event_queue.begin(), event_queue.end(), std::greater<Event>());
}

void TrafficGenerator::add_flow(const Flow& f) {
    flows.push_back(f);
}

void TrafficGenerator::schedule_flows(std::map<int, Node>& nodes_map) {
    node_map_ptr = &nodes_map;

    for (const auto& f : flows) {
        Event e;
        e.time = (int)f.start_time;
        e.type = "PACKET_GENERATE";

        e.callback = [this, f, &nodes_map]() {
            this->generate_packet(f, f.start_time, nodes_map);
        };

        schedule_event(e);
    }
}

void TrafficGenerator::generate_packet(const Flow& f, double time, std::map<int, Node>& nodes_map) {
    // Stop condition: flow duration exceeded
    if (time > f.start_time + f.duration) {
        return;
    }

    // Verify source node exists
    if (nodes_map.find(f.source) == nodes_map.end()) {
        std::cerr << "[TrafficGenerator] ERROR: Source node " << f.source << " not found\n";
        return;
    }

    // Create packet
    Packet pkt(
        f.source,
        f.destination,
        f.packet_size,
        f.use_tcp ? "TCP" : "UDP",
        (int)time,
        64  // TTL
    );

    // Send via source node
    Node& src_node = nodes_map.at(f.source);
    src_node.send_packet(pkt);

    std::cout << "[TrafficGenerator] Generated packet: src=" << f.source
              << " dst=" << f.destination
              << " size=" << f.packet_size
              << " time=" << time << std::endl;

    // Schedule next packet based on traffic type
    double next_time = time;

    if (f.type == TrafficType::CONSTANT) {
        // Deterministic interval
        next_time = time + (1.0 / f.rate);
    }
    else if (f.type == TrafficType::RANDOM) {
        // Exponential inter-arrival (Poisson traffic)
        static std::mt19937 gen(std::random_device{}());
        std::exponential_distribution<double> dist(f.rate);
        next_time = time + dist(gen);
    }
    else if (f.type == TrafficType::BURST) {
        // Alternate between burst rate and silence
        static std::mt19937 gen(std::random_device{}());
        std::uniform_real_distribution<double> u(0.0, 1.0);

        // 50% chance: high rate packet
        if (u(gen) < 0.5) {
            next_time = time + (1.0 / f.burst_rate);
        }
        // 50% chance: gap (silence)
        else {
            next_time = time + f.burst_duration;
        }
    }

    // Schedule next packet
    schedule_next_packet(f, next_time, nodes_map);
}

void TrafficGenerator::schedule_next_packet(const Flow& f, double next_time, std::map<int, Node>& nodes_map) {
    // Do not schedule if beyond flow duration
    if (next_time > f.start_time + f.duration) {
        return;
    }

    Event e;
    e.time = (int)next_time;
    e.type = "PACKET_GENERATE";

    e.callback = [this, f, next_time, &nodes_map]() {
        this->generate_packet(f, next_time, nodes_map);
    };

    schedule_event(e);
}
