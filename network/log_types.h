#ifndef NETWORK_LOG_TYPES_H
#define NETWORK_LOG_TYPES_H

#include <string>
#include <vector>
using namespace std;

// Log output levels
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

// Event types for logging (separate from simulation events)
enum class LogEventType {
    PACKET_CREATED,
    PACKET_FORWARDED,
    PACKET_DELIVERED,
    PACKET_DROPPED,
    ROUTING_DECISION,
    QUEUE_ENQUEUE,
    QUEUE_DEQUEUE,
    QUEUE_DROP
};

// Single log entry
struct LogEntry {
    int timestamp;
    int packet_id;
    int source_node;
    int dest_node;
    int current_node;
    int next_hop;
    LogEventType event;
    LogLevel level;
    string message;
    vector<int> path_history;  // packet path at time of log
    
    LogEntry()
        : timestamp(0), packet_id(-1), source_node(-1), dest_node(-1),
          current_node(-1), next_hop(-1), event(LogEventType::PACKET_CREATED),
          level(LogLevel::INFO), message(""), path_history() {}
};

#endif // NETWORK_LOG_TYPES_H
