#ifndef NETWORK_QUEUE_TYPES_H
#define NETWORK_QUEUE_TYPES_H

#include <climits>

// ============================================================================
// Queue Packet Metadata - Lightweight packet representation for queueing
// ============================================================================
// We store packet IDs and metadata rather than full Packet structs
// to avoid duplication and maintain clean separation of concerns

struct QueuedPacketInfo {
    int packet_id;           // Unique packet identifier from MetricsManager
    int source_node;         // Source node ID
    int destination_node;    // Final destination
    int enqueue_time;        // SimulationClock time when packet entered queue
    int dequeue_time;        // SimulationClock time when packet left queue (-1 if not yet)
    int queue_delay;         // dequeue_time - enqueue_time
    int priority;            // Queue priority (0 = normal)
    
    QueuedPacketInfo()
        : packet_id(-1), source_node(-1), destination_node(-1),
          enqueue_time(0), dequeue_time(-1), queue_delay(-1), priority(0) {}
    
    QueuedPacketInfo(int pkt_id, int src, int dst, int en_time)
        : packet_id(pkt_id), source_node(src), destination_node(dst),
          enqueue_time(en_time), dequeue_time(-1), queue_delay(-1), priority(0) {}
};

// ============================================================================
// Queue Statistics Structure
// ============================================================================
struct QueueStats {
    int total_enqueued;        // Total packets added to queue
    int total_dequeued;        // Total packets removed from queue
    int total_dropped_overflow; // Packets dropped due to capacity overflow
    
    double avg_queue_delay;    // Average time in queue
    double max_queue_delay;    // Longest wait time
    int min_queue_delay;       // Shortest wait time
    
    int current_size;          // Current queue length
    int max_observed_size;     // Peak queue depth
    
    QueueStats()
        : total_enqueued(0), total_dequeued(0), total_dropped_overflow(0),
          avg_queue_delay(0.0), max_queue_delay(0), min_queue_delay(INT_MAX),
          current_size(0), max_observed_size(0) {}
};

#endif // NETWORK_QUEUE_TYPES_H
