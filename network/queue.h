#ifndef NETWORK_QUEUE_H
#define NETWORK_QUEUE_H

#include "queue_types.h"
#include <queue>
#include <map>
#include <vector>
#include <string>

using namespace std;
// ============================================================================
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
    static Queue* getInstance();
    static void destroyInstance();
    void setDefaultCapacity(int capacity);
    void setNodeCapacity(int node_id, int capacity);
    int getCapacity(int node_id) const;
    
    void enableDebug(bool enable) { debug_enabled = enable; }
    /// Enqueue a packet at a specific node
    bool enqueue(int node_id, int packet_id, int src_node, int dst_node, int current_time);
    
    /// Dequeue a packet from a specific node's queue
    int dequeue(int node_id, int current_time);
    
    int getQueueSize(int node_id) const;
    
    bool isEmpty(int node_id) const;
    
    bool isFull(int node_id) const;
    
    /// Get all packet IDs currently in a queue (for debugging)
    vector<int> peekQueue(int node_id) const;
    
    
    QueueStats getNodeStats(int node_id) const;
    
    void printQueueStats(int node_id) const;
    
    void printAllQueuesStats() const;
    
    void exportQueuesCSV(const string& filename);
    
    void clearQueue(int node_id);
    
    void clearAllQueues();
};

#endif // NETWORK_QUEUE_H
