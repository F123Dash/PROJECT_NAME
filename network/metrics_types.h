#ifndef NETWORK_METRICS_TYPES_H
#define NETWORK_METRICS_TYPES_H

#include <string>
#include <vector>
using namespace std;

// ============================================================================
// Per-Packet Metrics
// ============================================================================
enum class PacketStatus {
    CREATED,             // Packet initialized
    IN_TRANSIT,          // Traveling through network
    DELIVERED,           // Successfully reached destination
    DROPPED_TTL,         // TTL expired
    DROPPED_NO_ROUTE,    // No route found
    DROPPED_ERROR        // Other error
};

struct PerPacketMetrics {
    int packet_id;
    int source;
    int destination;
    int creation_time;
    int delivery_time;           // -1 if not delivered
    int latency;                 // -1 if not delivered
    int first_hop_time;          // -1 if never hopped
    int hop_count;
    PacketStatus status;
    string status_reason;
    
    PerPacketMetrics()
        : packet_id(-1), source(-1), destination(-1), creation_time(0),
          delivery_time(-1), latency(-1), first_hop_time(-1), hop_count(0),
          status(PacketStatus::CREATED), status_reason("") {}
};

// ============================================================================
// Global Aggregated Metrics
// ============================================================================
struct GlobalMetrics {
    // Counts
    int total_packets;
    int delivered;
    int dropped;
    int dropped_ttl;
    int dropped_no_route;
    
    // Latency
    double avg_latency;
    double min_latency;
    double max_latency;
    double jitter;  // Standard deviation
    
    // Throughput & Hops
    double throughput;  // packets per time unit
    double avg_hops;
    
    // Loss
    double loss_rate;  // percentage
    
    GlobalMetrics()
        : total_packets(0), delivered(0), dropped(0), dropped_ttl(0), dropped_no_route(0),
          avg_latency(0.0), min_latency(INT_MAX), max_latency(0), jitter(0.0),
          throughput(0.0), avg_hops(0.0), loss_rate(0.0) {}
};

#endif // NETWORK_METRICS_TYPES_H
