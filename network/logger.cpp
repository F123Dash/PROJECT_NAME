#include "logger.h"
#include "../src/simulator_clock.h"
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace std;

Logger* Logger::instance = nullptr;

Logger::Logger()
    : log_file(""), file_enabled(false), console_enabled(true),
      min_level(LogLevel::INFO), total_logs(0) {
}

Logger* Logger::getInstance() {
    if (instance == nullptr) {
        instance = new Logger();
    }
    return instance;
}

void Logger::destroyInstance() {
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;
    }
}

void Logger::initialize(const string& filename, bool console) {
    console_enabled = console;
    total_logs = 0;
    log_buffer.clear();
    packet_traces.clear();
    
    if (!filename.empty()) {
        log_file = filename;
        file_enabled = true;
        cout << "[Logger] File output enabled: " << filename << endl;
    }
    
    if (console_enabled) {
        cout << "[Logger] Console output enabled" << endl;
    }
}

void Logger::setLogLevel(LogLevel level) {
    min_level = level;
}

void Logger::finalize() {
    cout << "[Logger] Finalized. Total logs: " << total_logs << endl;
    if (file_enabled) {
        exportToFile();
    }
}

void Logger::logPacketCreated(int packet_id, int src, int dst, int timestamp) {
    LogEntry entry;
    entry.timestamp = timestamp;
    entry.packet_id = packet_id;
    entry.source_node = src;
    entry.dest_node = dst;
    entry.event = EventType::PACKET_CREATED;
    entry.level = LogLevel::INFO;
    
    stringstream ss;
    ss << "Packet " << packet_id << " created: " << src << " -> " << dst;
    entry.message = ss.str();
    
    log_buffer.push_back(entry);
    total_logs++;
    
    if (console_enabled) {
        cout << "[LOG] T=" << timestamp << " " << entry.message << endl;
    }
}

void Logger::logPacketForwarded(int packet_id, int from_node, int to_node, int next_hop, int timestamp) {
    LogEntry entry;
    entry.timestamp = timestamp;
    entry.packet_id = packet_id;
    entry.current_node = from_node;
    entry.next_hop = next_hop;
    entry.event = EventType::PACKET_FORWARDED;
    entry.level = LogLevel::DEBUG;
    
    stringstream ss;
    ss << "Packet " << packet_id << " forwarded from node " << from_node << " to " << next_hop;
    entry.message = ss.str();
    
    log_buffer.push_back(entry);
    total_logs++;
    
    if (console_enabled && min_level <= LogLevel::DEBUG) {
        cout << "[LOG] T=" << timestamp << " " << entry.message << endl;
    }
}

void Logger::logPacketDelivered(int packet_id, int at_node, const vector<int>& path, int timestamp) {
    LogEntry entry;
    entry.timestamp = timestamp;
    entry.packet_id = packet_id;
    entry.current_node = at_node;
    entry.path_history = path;
    entry.event = EventType::PACKET_DELIVERED;
    entry.level = LogLevel::INFO;
    
    stringstream ss;
    ss << "Packet " << packet_id << " delivered at node " << at_node << " (path: ";
    for (size_t i = 0; i < path.size(); i++) {
        ss << path[i];
        if (i < path.size() - 1) ss << " -> ";
    }
    ss << ")";
    entry.message = ss.str();
    
    log_buffer.push_back(entry);
    total_logs++;
    
    if (console_enabled) {
        cout << "[LOG] T=" << timestamp << " " << entry.message << endl;
    }
}

void Logger::logPacketDropped(int packet_id, int at_node, const string& reason, int timestamp) {
    LogEntry entry;
    entry.timestamp = timestamp;
    entry.packet_id = packet_id;
    entry.current_node = at_node;
    entry.event = EventType::PACKET_DROPPED;
    entry.level = LogLevel::WARNING;
    
    stringstream ss;
    ss << "Packet " << packet_id << " dropped at node " << at_node << " (" << reason << ")";
    entry.message = ss.str();
    
    log_buffer.push_back(entry);
    total_logs++;
    
    if (console_enabled) {
        cout << "[LOG] T=" << timestamp << " " << entry.message << endl;
    }
}

void Logger::logRoutingDecision(int packet_id, int src_node, int dst_node, int next_hop, int timestamp) {
    LogEntry entry;
    entry.timestamp = timestamp;
    entry.packet_id = packet_id;
    entry.source_node = src_node;
    entry.dest_node = dst_node;
    entry.current_node = src_node;
    entry.next_hop = next_hop;
    entry.event = EventType::ROUTING_DECISION;
    entry.level = LogLevel::DEBUG;
    
    stringstream ss;
    ss << "Route decision: pkt " << packet_id << " at node " << src_node 
       << " to dst " << dst_node << " via " << next_hop;
    entry.message = ss.str();
    
    log_buffer.push_back(entry);
    total_logs++;
    
    if (console_enabled && min_level <= LogLevel::DEBUG) {
        cout << "[LOG] T=" << timestamp << " " << entry.message << endl;
    }
}

void Logger::logQueueEvent(int packet_id, EventType event, int node_id, int timestamp) {
    LogEntry entry;
    entry.timestamp = timestamp;
    entry.packet_id = packet_id;
    entry.current_node = node_id;
    entry.event = event;
    entry.level = LogLevel::DEBUG;
    
    stringstream ss;
    if (event == EventType::QUEUE_ENQUEUE) {
        ss << "Packet " << packet_id << " enqueued at node " << node_id;
    } else if (event == EventType::QUEUE_DEQUEUE) {
        ss << "Packet " << packet_id << " dequeued from node " << node_id;
    } else {
        ss << "Queue event for packet " << packet_id << " at node " << node_id;
    }
    entry.message = ss.str();
    
    log_buffer.push_back(entry);
    total_logs++;
    
    if (console_enabled && min_level <= LogLevel::DEBUG) {
        cout << "[LOG] T=" << timestamp << " " << entry.message << endl;
    }
}

void Logger::recordHop(int packet_id, int node_id) {
    packet_traces[packet_id].push_back(node_id);
}

vector<int> Logger::getPacketTrace(int packet_id) const {
    auto it = packet_traces.find(packet_id);
    if (it != packet_traces.end()) {
        return it->second;
    }
    return {};
}

void Logger::printLogs() const {
    cout << "\n";
    cout << "============================================================" << endl;
    cout << "                    SIMULATOR LOGS" << endl;
    cout << "============================================================" << endl;
    cout << "Total log entries: " << total_logs << endl;
    cout << "\n";
    
    for (const auto& entry : log_buffer) {
        cout << "T=" << setw(6) << setfill('0') << entry.timestamp
             << setfill(' ') << " | ";
        
        string level_str;
        if (entry.level == LogLevel::DEBUG) level_str = "DEBUG";
        else if (entry.level == LogLevel::INFO) level_str = "INFO";
        else if (entry.level == LogLevel::WARNING) level_str = "WARN";
        else level_str = "ERROR";
        
        cout << setw(7) << level_str << " | ";
        cout << entry.message << endl;
    }
    
    cout << "\n============================================================\n" << endl;
}

void Logger::exportToFile() {
    if (log_file.empty()) return;
    
    ofstream file(log_file);
    if (!file.is_open()) {
        cerr << "[Logger] ERROR: Cannot open " << log_file << endl;
        return;
    }
    
    for (const auto& entry : log_buffer) {
        file << "T=" << entry.timestamp << " | "
             << entry.message << endl;
    }
    
    file.close();
    cout << "[Logger] Logs exported to " << log_file << endl;
}

void Logger::exportToCSV(const string& filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "[Logger] ERROR: Cannot open " << filename << endl;
        return;
    }
    
    // Header
    file << "Timestamp,PacketID,Event,SourceNode,DestNode,CurrentNode,NextHop,Message\n";
    
    for (const auto& entry : log_buffer) {
        string event_str;
        if (entry.event == EventType::PACKET_CREATED) event_str = "CREATED";
        else if (entry.event == EventType::PACKET_FORWARDED) event_str = "FORWARDED";
        else if (entry.event == EventType::PACKET_DELIVERED) event_str = "DELIVERED";
        else if (entry.event == EventType::PACKET_DROPPED) event_str = "DROPPED";
        else if (entry.event == EventType::ROUTING_DECISION) event_str = "ROUTING";
        else event_str = "OTHER";
        
        file << entry.timestamp << ","
             << entry.packet_id << ","
             << event_str << ","
             << entry.source_node << ","
             << entry.dest_node << ","
             << entry.current_node << ","
             << entry.next_hop << ","
             << "\"" << entry.message << "\"\n";
    }
    
    file.close();
    cout << "[Logger] Logs exported to CSV: " << filename << endl;
}
