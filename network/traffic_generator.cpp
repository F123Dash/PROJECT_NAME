#include "traffic_generator.h"
#include "../engine/node.h"
#include "../engine/event_queue.h"
#include "../network/metrics.h"
#include "../network/logger.h"
#include "../network/debug.h"
#include <random>
#include <iostream>

// Use the global schedule_event from event_queue.cpp
extern void schedule_event(Event e);

void TrafficGenerator::add_flow(const Flow& f) {
    flows.push_back(f);
}

void TrafficGenerator::schedule_flows(std::map<int, Node*>& nodes_map) {
    node_map_ptr = &nodes_map;

    for (const auto& f : flows) {
        Event e;
        e.time = f.start_time;  // No cast - preserve precision
        e.type = "PACKET_GENERATE";

        e.callback = [this, f, &nodes_map]() {
            this->generate_packet(f, f.start_time, nodes_map);
        };

        schedule_event(e);
    }
}

void TrafficGenerator::generate_packet(const Flow& f, double time, std::map<int, Node*>& nodes_map) {
    // Stop condition: flow duration exceeded
    if (time > f.start_time + f.duration) {
        return;
    }

    // Verify source node exists
    ASSERT(nodes_map.find(f.source) != nodes_map.end(),
           "Source node " + std::to_string(f.source) + " not found");

    // Record packet creation in metrics to get packet_id
    int packet_id = MetricsManager::getInstance()->onPacketCreated(
        f.source, f.destination, (int)time, f.packet_size
    );

    // Create packet with the packet_id
    Packet pkt(
        packet_id,
        f.source,
        f.destination,
        f.packet_size,
        f.use_tcp ? "TCP" : "UDP",
        (int)time,
        64  // TTL
    );

    // Send via source node (use pointer)
    Node* src_node = nodes_map.at(f.source);
    ASSERT(src_node != nullptr, "Source node pointer is NULL");
    
    if (DEBUG_MODE) {
        log(LogLevel::INFO,
            "Generated packet " + std::to_string(packet_id) +
            ": " + std::to_string(f.source) +
            " -> " + std::to_string(f.destination) +
            " (size=" + std::to_string(f.packet_size) + " bytes)");
    }
    
    src_node->send_packet(pkt);

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

void TrafficGenerator::schedule_next_packet(const Flow& f, double next_time, std::map<int, Node*>& nodes_map) {
    // Do not schedule if beyond flow duration
    if (next_time > f.start_time + f.duration) {
        return;
    }

    Event e;
    e.time = next_time;  // No cast - preserve precision
    e.type = "PACKET_GENERATE";

    e.callback = [this, f, next_time, &nodes_map]() {
        this->generate_packet(f, next_time, nodes_map);
    };

    schedule_event(e);
}