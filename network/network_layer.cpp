#include "network_layer.h"
#include "../engine/node.h"
#include "../engine/integration.h"
#include "../engine/event_types.h"
#include "../engine/event_queue.h"
#include "../graphs/graphs.h"
#include "../graphs/link.h"
#include "../network/metrics.h"
#include "../network/logger.h"
#include "../network/debug.h"
#include <iostream>
#include <algorithm>

// Get logger instance for event tracking
static Logger* event_logger = nullptr;

NetworkLayer::NetworkLayer(Node* n) : node(n) {
    ASSERT(n != nullptr, "Node pointer is NULL in NetworkLayer");
    // Initialize logger on first use
    if (!event_logger) {
        event_logger = Logger::getInstance();
    }
}

// Called by Transport Layer to send a packet
void NetworkLayer::send(Packet p) {
    extern double current_time;
    
    std::cout << "[NetworkLayer] SEND: src=" << p.source
              << " dst=" << p.destination
              << " ttl=" << p.ttl
              << " size=" << p.size << std::endl;
    
    // Log packet send event
    if (event_logger) {
        event_logger->logPacketCreated(p.packet_id, p.source, p.destination, (int)current_time);
    }
    
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
        
        // Log drop event
        if (event_logger) {
            event_logger->logPacketDropped(p.packet_id, node->id, "TTL_EXPIRED", (int)current_time);
        }
        
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
        
        // Log delivery event
        if (event_logger) {
            event_logger->logPacketDelivered(p.packet_id, node->id, p.path_history, (int)current_time);
        }
        
        p.print_info();
        
        // Send ACK back to source
        extern bool ENABLE_RETRANSMISSION;
        if (ENABLE_RETRANSMISSION) {
            sendAck(p.packet_id, p.source);
        }
        
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
        
        // Log drop event
        if (event_logger) {
            event_logger->logPacketDropped(p.packet_id, node->id, "NO_ROUTE", (int)current_time);
        }
        
        return;
    }

    // Get next hop node and deliver packet
    extern Integration* GLOBAL_INTEGRATION;
    if (GLOBAL_INTEGRATION) {
        Node* next_node = GLOBAL_INTEGRATION->get_node(next_hop);
        ASSERT(next_node != nullptr, "Next hop node is NULL");
        ASSERT(next_node->network_layer != nullptr, "Network layer missing on next node");
        
        // Calculate travel time based on link properties
        Graph* graph = GLOBAL_INTEGRATION->graph;
        double travel_time = 2.0;  // Default fallback
        
        if (graph) {
            Link* link = graph->getLinkProperties(node->id, next_hop);
            if (link) {
                travel_time = link->calculateTravelTime(p.size);
                if (DEBUG_MODE) {
                    log(LogLevel::DEBUG,
                        "Scheduled packet " + std::to_string(p.packet_id) +
                        " to arrive at node " + std::to_string(next_hop) +
                        " in " + std::to_string(travel_time) + " ms");
                }
            }
        }
        
        // Schedule packet arrival with calculated travel time
        schedulePacketArrival(p, next_hop, travel_time);
        
        // Log forwarding decision
        extern double current_time;
        if (event_logger) {
            event_logger->logRoutingDecision(p.packet_id, node->id, p.destination, next_hop, (int)current_time);
        }
    }
}

// Schedule packet arrival (delayed delivery based on link properties)
void NetworkLayer::schedulePacketArrival(Packet p, int next_hop, double travel_time, bool is_ack) {
    extern void schedule_event(Event e);
    extern double current_time;
    extern Integration* GLOBAL_INTEGRATION;
    
    // Create a callback that will deliver the packet
    Event e;
    e.time = current_time + travel_time;
    e.type = is_ack ? "ACK_ARRIVAL" : "PACKET_ARRIVAL";
    e.callback = [p, next_hop, is_ack]() {
        if (GLOBAL_INTEGRATION) {
            Node* dest_node = GLOBAL_INTEGRATION->get_node(next_hop);
            if (dest_node && dest_node->network_layer) {
                if (is_ack) {
                    dest_node->network_layer->receiveAck(p.packet_id);
                } else {
                    dest_node->network_layer->receive(p);
                }
            }
        }
    };
    
    // Use the proper global schedule_event function
    schedule_event(e);
    
    // Only schedule retransmission timeout for data packets (not ACKs), 
    // and only on the sending side (when traveling towards destination)
    if (!is_ack) {
        extern bool ENABLE_RETRANSMISSION;
        extern double RTX_TIMEOUT_MS;
        if (ENABLE_RETRANSMISSION && p.source == node->id) {
            // Only sender should schedule timeouts
            scheduleRetransmissionTimeout(p, RTX_TIMEOUT_MS);
        }
    }
}

// Schedule retransmission timeout for a packet
void NetworkLayer::scheduleRetransmissionTimeout(Packet p, double timeout_ms) {
    extern void schedule_event(Event e);
    extern double current_time;
    
    Event timeout_event;
    timeout_event.time = current_time + (timeout_ms / 1000.0);
    timeout_event.type = "RETRANSMISSION_TIMEOUT";
    timeout_event.callback = [p, this]() {
        this->handleRetransmissionTimeout(p);
    };
    
    schedule_event(timeout_event);
}

// Handle retransmission timeout - only retransmit if ACK not received
void NetworkLayer::handleRetransmissionTimeout(Packet p) {
    extern bool ENABLE_RETRANSMISSION;
    extern int MAX_RETRANSMISSIONS;
    extern double current_time;
    
    if (!ENABLE_RETRANSMISSION) return;
    
    // If ACK was already received, don't retransmit
    if (acknowledged_packets.count(p.packet_id) > 0) {
        std::cout << "[NetworkLayer] TIMEOUT for packet " << p.packet_id 
                  << " but ACK already received - no retransmission needed" << std::endl;
        return;
    }
    
    int& retry_count = retransmit_count[p.packet_id];
    
    if (retry_count >= MAX_RETRANSMISSIONS) {
        // Max retries exceeded - packet definitely lost
        std::cout << "[NetworkLayer] RETRANSMISSION LIMIT EXCEEDED for packet " 
                  << p.packet_id << " (retries=" << retry_count << ")" << std::endl;
        MetricsManager::getInstance()->onPacketDropped_TTL(p.packet_id, (int)current_time);
        return;
    }
    
    // Increment retry counter and resend
    retry_count++;
    std::cout << "[NetworkLayer] RETRANSMITTING packet " << p.packet_id 
              << " (attempt " << retry_count << "/" << MAX_RETRANSMISSIONS << ")" << std::endl;
    
    // Resend the packet via forward
    forward(p);
    
    // Schedule another timeout for this retry
    extern double RTX_TIMEOUT_MS;
    scheduleRetransmissionTimeout(p, RTX_TIMEOUT_MS);
}

// Send ACK back to source node
void NetworkLayer::sendAck(int packet_id, int source_node) {
    if (!node) return;
    
    extern double current_time;
    
    std::cout << "[NetworkLayer] SENDING ACK for packet " << packet_id 
              << " back to source node " << source_node << std::endl;
    
    // Log ACK event
    if (event_logger) {
        event_logger->logRoutingDecision(packet_id, node->id, source_node, source_node, (int)current_time);
    }
    
    // Create a minimal ACK packet
    extern double current_time;
    Packet ack_pkt(
        packet_id,              // packet_id
        node->id,               // source (this node sending the ACK)
        source_node,            // destination (back to originator)
        64,                     // size (minimal ACK)
        "TCP",                  // protocol
        (int)current_time,      // timestamp
        64                      // ttl
    );
    
    // Schedule ACK arrival at source with minimal travel time
    extern Integration* GLOBAL_INTEGRATION;
    
    if (GLOBAL_INTEGRATION) {
        Graph* graph = GLOBAL_INTEGRATION->graph;
        if (graph) {
            Link* link = graph->getLinkProperties(node->id, source_node);
            if (link) {
                double ack_travel_time = link->calculateTravelTime(64);
                schedulePacketArrival(ack_pkt, source_node, ack_travel_time, true);
                return;
            }
        }
    }
    
    // Fallback: schedule with default travel time
    schedulePacketArrival(ack_pkt, source_node, 2.0, true);
}

// Process received ACK - mark packet as successfully acknowledged
void NetworkLayer::receiveAck(int packet_id) {
    if (!node) return;
    
    extern double current_time;
    
    std::cout << "[NetworkLayer] RECEIVED ACK for packet " << packet_id << std::endl;
    
    // Log ACK receipt
    if (event_logger) {
        event_logger->recordHop(packet_id, node->id);
    }
    
    // Mark as acknowledged
    acknowledged_packets.insert(packet_id);
    
    // If we scheduled a timeout for this packet, we could cancel it here
    // (Currently we rely on checking acknowledged_packets in handleRetransmissionTimeout)
}