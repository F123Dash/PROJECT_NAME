#ifndef NETWORK_QUEUE_H
#define NETWORK_QUEUE_H

#include "queue_types.h"
#include <queue>
#include <map>
#include <vector>
#include <string>

using namespace std;

// ============================================================================
// Queue Manager - Singleton Pattern
//
// Manages FIFO packet queues for each node in the network.
// 
// Key Features:
//  - Per-node FIFO queues
//  - Configurable max capacity per queue
//  - Automatic overflow drop with metrics reporting
//  - Queue delay tracking
//  - Statistics collection
//
// Integration Points:
//  1. enqueue(): called from NetworkLayer::forward() when routing succeeds
//  2. dequeue(): called from simulation main loop for each node
//  3. Reports to MetricsManager for packet drops and delays
// ============================================================================

class Queue {
private:
    static Queue* instance;
    
    // Per-node packet queues
    // node_id → queue of QueuedPacketInfo
    map<int, queue<QueuedPacketInfo>> node_queues;
    
    // Per-node statistics
    map<int, QueueStats> node_stats;
    
    // Configuration
    map<int, int> node_capacities;  // node_id → max_capacity (default 100)
    int default_capacity;
    
    // Global stats
    int total_drops;
    bool debug_enabled;
    
    // Private constructor
    Queue();

public:
    // ========================================================================
    // Singleton Access
    // ========================================================================
    static Queue* getInstance();
    static void destroyInstance();
    
    // ========================================================================
    // Configuration
    // ========================================================================
    void setDefaultCapacity(int capacity);
    void setNodeCapacity(int node_id, int capacity);
    int getCapacity(int node_id) const;
    
    void enableDebug(bool enable) { debug_enabled = enable; }
    
    // ========================================================================
    // HOOK METHODS - Core Operations
    // ========================================================================
    
    /// Enqueue a packet at a specific node
    /// Called from: NetworkLayer::forward() when packet is ready to be sent
    ///
    /// Parameters:
    ///   node_id: ID of the node where packet is queued
    ///   packet_id: Unique packet ID from MetricsManager
    ///   src_node: Source node of packet
    ///   dst_node: Destination node of packet
    ///   current_time: Current simulation time (from SimulationClock::getTime())
    ///
    /// Returns: true if enqueued successfully, false if dropped (overflow)
    ///
    /// Side Effects:
    ///   - If queue is full: drops packet and reports to MetricsManager
    ///   - Updates queue statistics
    ///   - Optional debug output if enabled
    bool enqueue(int node_id, int packet_id, int src_node, int dst_node, int current_time);
    
    /// Dequeue a packet from a specific node's queue
    /// Called from: Main simulation loop periodically
    ///
    /// Parameters:
    ///   node_id: ID of the node to dequeue from
    ///   current_time: Current simulation time (for delay calculation)
    ///
    /// Returns: packet_id of dequeued packet, or -1 if queue is empty
    ///
    /// Side Effects:
    ///   - Records dequeue time
    ///   - Calculates queue delay
    ///   - Reports delay to MetricsManager
    ///   - Updates statistics
    ///   - Optional debug output if enabled
    int dequeue(int node_id, int current_time);
    
    // ========================================================================
    // Query Methods
    // ========================================================================
    
    int getQueueSize(int node_id) const;
    
    bool isEmpty(int node_id) const;
    
    bool isFull(int node_id) const;
    
    /// Get all packet IDs currently in a queue (for debugging)
    vector<int> peekQueue(int node_id) const;
    
    // ========================================================================
    // Statistics and Reporting
    // ========================================================================
    
    QueueStats getNodeStats(int node_id) const;
    
    void printQueueStats(int node_id) const;
    
    void printAllQueuesStats() const;
    
    void exportQueuesCSV(const string& filename);
    
    // ========================================================================
    // Cleanup/Reset
    // ========================================================================
    
    void clearQueue(int node_id);
    
    void clearAllQueues();
};

#endif // NETWORK_QUEUE_H
