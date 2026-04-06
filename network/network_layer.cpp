#include "network_layer.h"
#include "../src/node.h"
#include <iostream>

NetworkLayer::NetworkLayer(Node* n) : node(n) {
    if (!node) {
        std::cerr << "[NetworkLayer] ERROR: Node pointer is NULL\n";
    }
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

    std::cout << "[NetworkLayer] RECEIVE at node=" << node->id
              << " from=" << p.source
              << " dst=" << p.destination
              << " ttl=" << p.ttl
              << " hops=" << p.path_history.size() << std::endl;

    // Step 3: Check if TTL expired
    if (p.ttl <= 0) {
        std::cout << "[NetworkLayer] TTL EXPIRED! Dropping packet at node " << node->id << std::endl;
        // TODO: Update metrics counter for dropped packets
        return;
    }

    // Step 4: Check if destination reached
    if (p.destination == node->id) {
        std::cout << "[NetworkLayer] DESTINATION REACHED at node " << node->id << std::endl;
        // TODO: Update metrics - latency = current_time - p.timestamp
        // Deliver to transport layer
        // node->transport->receive(p);  // if transport interface exists
        p.print_info();
        return;
    }

    // Step 5: Forward to next hop
    std::cout << "[NetworkLayer] FORWARDING from node " << node->id << std::endl;
    forward(p);
}

// Core forwarding logic: lookup routing table and send via node
void NetworkLayer::forward(Packet p) {
    if (!node) return;

    // Look up next hop in routing table
    int next_hop = node->get_next_hop(p.destination);

    std::cout << "[NetworkLayer] FORWARD: current_node=" << node->id
              << " dst=" << p.destination
              << " next_hop=" << next_hop << std::endl;

    // No route available: drop packet
    if (next_hop == -1) {
        std::cout << "[NetworkLayer] NO ROUTE to destination " << p.destination
                  << " at node " << node->id << ". Dropping packet." << std::endl;
        // TODO: Update metrics counter for dropped packets (unreachable)
        return;
    }

    // Send to link layer via node's packet queue
    node->send_packet(p);
}
