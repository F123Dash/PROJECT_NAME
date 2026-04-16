#ifndef NETWORK_METRICS_H
#define NETWORK_METRICS_H

#include "metrics_types.h"
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <cmath>

using namespace std;

// ============================================================================
// MetricsManager - Singleton Pattern
// 
// Non-intrusive tracking of packet metrics during simulation.
// Hook points are minimal: 5 method calls at key locations in NetworkLayer.
// ============================================================================

class MetricsManager {
private:
    static MetricsManager* instance;
    
    // Per-packet storage
    map<int, PerPacketMetrics> packets;
    
    // Auto-increment packet ID
    int next_packet_id;
    
    // Simulation timing
    int sim_start_time;
    int sim_end_time;
    
    // Output
    string csv_filename;
    bool csv_enabled;

    // Private constructor (singleton)
    MetricsManager();

public:
    // ========================================================================
    // Singleton Access
    // ========================================================================
    static MetricsManager* getInstance();
    static void destroyInstance();
    
    // ========================================================================
    // Simulation Lifecycle
    // ========================================================================
    void initialize(int start_time = 0, const string& csv_file = "");
    void finalize(int end_time);
    
    // ========================================================================
    // HOOK METHODS - Called from NetworkLayer
    // (These are the insertion points)
    // ========================================================================
    
    /// Call from Packet constructor when creating a new packet
    /// Returns: unique packet_id to store in packet.packet_id
    int onPacketCreated(int src, int dst, int creation_time, int size = 0);
    
    /// Call from NetworkLayer::receive() AFTER p.record_hop()
    /// Increments hop count for this packet
    void onPacketHop(int packet_id, int node_id, int current_time);
    
    /// Call from NetworkLayer::receive() when packet reaches destination
    /// Marks packet as delivered and records latency
    void onPacketDelivered(int packet_id, int delivery_time, int final_hop_count);
    
    /// Call from NetworkLayer::receive() when TTL expires
    /// Records drop reason
    void onPacketDropped_TTL(int packet_id, int current_time);
    
    /// Call from NetworkLayer::forward() when no route found
    /// Records drop reason
    void onPacketDropped_NoRoute(int packet_id, int current_time);
    
    /// Call from Queue::enqueue() when packet dropped due to overflow
    /// Records drop reason for queue system
    void onPacketDropped_QueueOverflow(int packet_id, int current_time);
    
    /// Call from Queue::dequeue() to record queue delay
    /// Records the time packet spent in queue
    void onQueueDelay(int packet_id, int delay_ms);
    
    // ========================================================================
    // Query Methods
    // ========================================================================
    const PerPacketMetrics* getPacketMetrics(int packet_id) const;
    int getTotalPackets() const { return packets.size(); }
    
    // ========================================================================
    // Metrics Computation
    // ========================================================================
    GlobalMetrics computeGlobalMetrics();
    
    // ========================================================================
    // Output Methods
    // ========================================================================
    void printMetricsReport();
    void exportMetricsCSV();
    void exportMetricsToFile(const string& filename);
    void exportLatencySeriesToJSON(std::ostream& out) const;
};

// Global metrics printer
void print_metrics();

#endif // NETWORK_METRICS_H
