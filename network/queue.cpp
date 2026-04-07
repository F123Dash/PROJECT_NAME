#include "queue.h"
#include "metrics.h"
#include "../src/simulator_clock.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <algorithm>

using namespace std;

// ============================================================================
// Static Member Initialization
// ============================================================================
Queue* Queue::instance = nullptr;

// ============================================================================
// Queue Implementation
// ============================================================================

Queue::Queue()
    : default_capacity(100),
      total_drops(0),
      debug_enabled(false) {
}

Queue* Queue::getInstance() {
    if (instance == nullptr) {
        instance = new Queue();
    }
    return instance;
}

void Queue::destroyInstance() {
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;
    }
}

// ============================================================================
// Configuration Methods
// ============================================================================

void Queue::setDefaultCapacity(int capacity) {
    if (capacity > 0) {
        default_capacity = capacity;
        cout << "[Queue] Default capacity set to " << capacity << endl;
    } else {
        cerr << "[Queue] Error: capacity must be > 0" << endl;
    }
}

void Queue::setNodeCapacity(int node_id, int capacity) {
    if (capacity > 0) {
        node_capacities[node_id] = capacity;
        cout << "[Queue] Node " << node_id << " capacity set to " << capacity << endl;
    } else {
        cerr << "[Queue] Error: capacity must be > 0" << endl;
    }
}

int Queue::getCapacity(int node_id) const {
    auto it = node_capacities.find(node_id);
    if (it != node_capacities.end()) {
        return it->second;
    }
    return default_capacity;
}

// ============================================================================
// Core Queue Operations
// ============================================================================

bool Queue::enqueue(int node_id, int packet_id, int src_node, int dst_node, int current_time) {
    int capacity = getCapacity(node_id);
    
    // Check if queue is full
    if ((int)node_queues[node_id].size() >= capacity) {
        cout << "[Queue] OVERFLOW at node " << node_id
             << " packet_id=" << packet_id << " (capacity=" << capacity << ")"
             << endl;
        
        // Report to metrics manager
        MetricsManager::getInstance()->onPacketDropped_QueueOverflow(packet_id, current_time);
        
        // Update statistics
        node_stats[node_id].total_dropped_overflow++;
        total_drops++;
        
        return false;  // Drop failed
    }
    
    // Create packet metadata
    QueuedPacketInfo info(packet_id, src_node, dst_node, current_time);
    
    // Enqueue
    node_queues[node_id].push(info);
    
    // Update statistics
    node_stats[node_id].total_enqueued++;
    node_stats[node_id].current_size = node_queues[node_id].size();
    node_stats[node_id].max_observed_size = max(
        node_stats[node_id].max_observed_size,
        node_stats[node_id].current_size
    );
    
    if (debug_enabled) {
        cout << "[Queue] ENQUEUE at node " << node_id
             << " packet_id=" << packet_id
             << " queue_size=" << node_stats[node_id].current_size
             << " time=" << current_time << endl;
    }
    
    return true;
}

int Queue::dequeue(int node_id, int current_time) {
    // Check if queue exists and has packets
    if (node_queues.find(node_id) == node_queues.end() ||
        node_queues[node_id].empty()) {
        return -1;  // Queue empty
    }
    
    // Get front packet
    QueuedPacketInfo& info = node_queues[node_id].front();
    
    // Calculate queue delay
    int delay = current_time - info.enqueue_time;
    info.dequeue_time = current_time;
    info.queue_delay = delay;
    
    int packet_id = info.packet_id;
    
    // Report to metrics
    MetricsManager::getInstance()->onQueueDelay(packet_id, delay);
    
    // Update statistics
    node_stats[node_id].total_dequeued++;
    node_stats[node_id].current_size = node_queues[node_id].size() - 1;
    
    // Update delay statistics
    if (node_stats[node_id].total_dequeued == 1) {
        // First packet
        node_stats[node_id].avg_queue_delay = delay;
        node_stats[node_id].max_queue_delay = delay;
        node_stats[node_id].min_queue_delay = delay;
    } else {
        // Incremental average update
        int total_old = node_stats[node_id].total_dequeued - 1;
        node_stats[node_id].avg_queue_delay = 
            (node_stats[node_id].avg_queue_delay * total_old + delay) / 
            node_stats[node_id].total_dequeued;
        
        node_stats[node_id].max_queue_delay = max(node_stats[node_id].max_queue_delay, (double)delay);
        node_stats[node_id].min_queue_delay = min(node_stats[node_id].min_queue_delay, delay);
    }
    
    // Remove from queue
    node_queues[node_id].pop();
    
    if (debug_enabled) {
        cout << "[Queue] DEQUEUE from node " << node_id
             << " packet_id=" << packet_id
             << " delay=" << delay
             << " queue_size=" << node_stats[node_id].current_size
             << endl;
    }
    
    return packet_id;
}

// ============================================================================
// Query Methods
// ============================================================================

int Queue::getQueueSize(int node_id) const {
    auto it = node_queues.find(node_id);
    if (it != node_queues.end()) {
        return it->second.size();
    }
    return 0;
}

bool Queue::isEmpty(int node_id) const {
    auto it = node_queues.find(node_id);
    if (it == node_queues.end()) {
        return true;
    }
    return it->second.empty();
}

bool Queue::isFull(int node_id) const {
    int capacity = getCapacity(node_id);
    return getQueueSize(node_id) >= capacity;
}

vector<int> Queue::peekQueue(int node_id) const {
    vector<int> packet_ids;
    
    auto it = node_queues.find(node_id);
    if (it == node_queues.end()) {
        return packet_ids;
    }
    
    // Convert queue to vector (non-destructive peek)
    queue<QueuedPacketInfo> temp_queue = it->second;
    while (!temp_queue.empty()) {
        packet_ids.push_back(temp_queue.front().packet_id);
        temp_queue.pop();
    }
    
    return packet_ids;
}

// ============================================================================
// Statistics Methods
// ============================================================================

QueueStats Queue::getNodeStats(int node_id) const {
    auto it = node_stats.find(node_id);
    if (it != node_stats.end()) {
        return it->second;
    }
    return QueueStats();
}

void Queue::printQueueStats(int node_id) const {
    QueueStats stats = getNodeStats(node_id);
    
    cout << "\n--- Queue Statistics for Node " << node_id << " ---" << endl;
    cout << "Total Enqueued:              " << stats.total_enqueued << endl;
    cout << "Total Dequeued:              " << stats.total_dequeued << endl;
    cout << "Total Dropped (Overflow):    " << stats.total_dropped_overflow << endl;
    cout << "Current Queue Size:          " << stats.current_size << endl;
    cout << "Max Observed Size:           " << stats.max_observed_size << endl;
    
    cout << fixed << setprecision(3);
    cout << "\nQueue Delay Statistics:" << endl;
    cout << "Average Delay (ms):          " << stats.avg_queue_delay << endl;
    cout << "Min Delay (ms):              " << (stats.min_queue_delay == INT_MAX ? 0 : stats.min_queue_delay) << endl;
    cout << "Max Delay (ms):              " << stats.max_queue_delay << endl;
    
    cout << "\nCapacity:                    " << getCapacity(node_id) << endl;
}

void Queue::printAllQueuesStats() const {
    cout << "\n";
    cout << "============================================================" << endl;
    cout << "              QUEUE SYSTEM STATISTICS" << endl;
    cout << "============================================================" << endl;
    
    if (node_stats.empty()) {
        cout << "No queue activity recorded." << endl;
        cout << "============================================================\n" << endl;
        return;
    }
    
    cout << "\nTotal Packets Dropped (Overflow): " << total_drops << endl;
    
    for (const auto& [node_id, stats] : node_stats) {
        printQueueStats(node_id);
    }
    
    cout << "\n============================================================\n" << endl;
}

void Queue::exportQueuesCSV(const string& filename) {
    ofstream file(filename);
    
    if (!file.is_open()) {
        cerr << "[Queue] ERROR: Could not open " << filename << endl;
        return;
    }
    
    // Header
    file << "NodeID,TotalEnqueued,TotalDequeued,TotalDropped,CurrentSize,"
         << "MaxObservedSize,AvgDelay(ms),MinDelay(ms),MaxDelay(ms),Capacity\n";
    
    // Per-node statistics
    for (const auto& [node_id, stats] : node_stats) {
        file << node_id << ","
             << stats.total_enqueued << ","
             << stats.total_dequeued << ","
             << stats.total_dropped_overflow << ","
             << stats.current_size << ","
             << stats.max_observed_size << ","
             << fixed << setprecision(3) << stats.avg_queue_delay << ","
             << (stats.min_queue_delay == INT_MAX ? 0 : stats.min_queue_delay) << ","
             << stats.max_queue_delay << ","
             << getCapacity(node_id) << "\n";
    }
    
    file.close();
    cout << "[Queue] Queue statistics exported to " << filename << endl;
}

// ============================================================================
// Cleanup Methods
// ============================================================================

void Queue::clearQueue(int node_id) {
    if (node_queues.find(node_id) != node_queues.end()) {
        // Create empty queue to replace it
        queue<QueuedPacketInfo> empty_queue;
        swap(node_queues[node_id], empty_queue);
        
        cout << "[Queue] Cleared queue for node " << node_id << endl;
    }
}

void Queue::clearAllQueues() {
    node_queues.clear();
    node_stats.clear();
    total_drops = 0;
    
    cout << "[Queue] Cleared all queues" << endl;
}
