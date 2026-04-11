#include "network_layer.h"
#include "../engine/node.h"
#include "../engine/integration.h"
#include "../network/metrics.h"
#include "../network/logger.h"
#include "../network/debug.h"
#include <iostream>

NetworkLayer::NetworkLayer(Node* n) : node(n) {
    ASSERT(n != nullptr, "Node pointer is NULL in NetworkLayer");
}

// Called by Transport Layer to send a packet
void NetworkLayer::send(Packet p) {
    std::cout << "[NetworkLayer] SEND: src=" << p.source
              << " dst=" << p.destination
              << " ttl=" << p.ttl
              << " size=" << p.size << std::endl;
    forward(p);
}

// Called when packet arrives at node
void NetworkLayer::receive(Packet p) {
    if (!node) return;

    // Step 1: Record this node in path history
    p.record_hop(node->id);

    // Step 2: Decrement TTL
    p.ttl--;

    if (DEBUG_MODE) {
        log(LogLevel::DEBUG,
            "Node " + std::to_string(node->id) +
            " received packet " + std::to_string(p.packet_id) +
            " from " + std::to_string(p.source) +
            " to " + std::to_string(p.destination) +
            " TTL=" + std::to_string(p.ttl));
    }

    std::cout << "[NetworkLayer] RECEIVE at node=" << node->id
              << " from=" << p.source
              << " dst=" << p.destination
              << " ttl=" << p.ttl
              << " hops=" << p.path_history.size() << std::endl;

    // Step 3: Check if TTL expired
    if (p.ttl <= 0) {
        log(LogLevel::ERROR,
            "Packet " + std::to_string(p.packet_id) +
            " TTL expired at node " + std::to_string(node->id));
        std::cout << "[NetworkLayer] TTL EXPIRED! Dropping packet at node " << node->id << std::endl;
        extern double current_time;
        MetricsManager::getInstance()->onPacketDropped_TTL(p.packet_id, (int)current_time);
        return;
    }

    // Step 4: Check if destination reached
    if (p.destination == node->id) {
        log(LogLevel::INFO,
            "Packet " + std::to_string(p.packet_id) +
            " delivered at node " + std::to_string(node->id) +
            " (hops=" + std::to_string(p.path_history.size()) + ")");
        std::cout << "[NetworkLayer] DESTINATION REACHED at node " << node->id << std::endl;
        // Packet delivered successfully - record in metrics
        extern double current_time;
        MetricsManager::getInstance()->onPacketDelivered(
            p.packet_id, (int)current_time, (int)p.path_history.size()
        );
        p.print_info();
        return;
    }

    // Step 5: Forward to next hop
    if (DEBUG_MODE) {
        log(LogLevel::DEBUG,
            "Forwarding packet " + std::to_string(p.packet_id) +
            " from node " + std::to_string(node->id));
    }
    std::cout << "[NetworkLayer] FORWARDING from node " << node->id << std::endl;
    forward(p);
}

// Core forwarding logic: lookup routing table and deliver to next hop node
void NetworkLayer::forward(Packet p) {
    if (!node) return;

    // Look up next hop in routing table
    int next_hop = node->get_next_hop(p.destination);

    if (DEBUG_MODE) {
        log(LogLevel::DEBUG,
            "Forwarding packet " + std::to_string(p.packet_id) +
            ": node " + std::to_string(node->id) +
            " -> " + std::to_string(next_hop));
    }

    std::cout << "[NetworkLayer] FORWARD: current_node=" << node->id
              << " dst=" << p.destination
              << " next_hop=" << next_hop << std::endl;

    // No route available: drop packet
    if (next_hop == -1) {
        log(LogLevel::ERROR,
            "Packet " + std::to_string(p.packet_id) +
            " dropped at node " + std::to_string(node->id) +
            " (no route to destination " + std::to_string(p.destination) + ")");
        std::cout << "[NetworkLayer] NO ROUTE to destination " << p.destination
                  << " at node " << node->id << ". Dropping packet." << std::endl;
        extern double current_time;
        MetricsManager::getInstance()->onPacketDropped_NoRoute(p.packet_id, (int)current_time);
        return;
    }

    // Get next hop node and deliver packet
    extern Integration* GLOBAL_INTEGRATION;
    if (GLOBAL_INTEGRATION) {
        Node* next_node = GLOBAL_INTEGRATION->get_node(next_hop);
        ASSERT(next_node != nullptr, "Next hop node is NULL");
        ASSERT(next_node->network_layer != nullptr, "Network layer missing on next node");
        
        // Deliver to next hop's network layer
        std::cout << "[NetworkLayer] DELIVERING to node " << next_hop << std::endl;
        next_node->network_layer->receive(p);
    }
}