#ifndef NETWORK_LOGGER_H
#define NETWORK_LOGGER_H

#include "log_types.h"
#include <map>
#include <fstream>
#include <vector>

using namespace std;

// Lightweight logging system (singleton)
// Hook points are minimal: 1 line per event
class Logger {
private:
    static Logger* instance;
    
    vector<LogEntry> log_buffer;
    map<int, vector<int>> packet_traces;  // packet_id → path taken
    
    string log_file;
    bool file_enabled;
    bool console_enabled;
    LogLevel min_level;
    int total_logs;
    
    Logger();
    
public:
    static Logger* getInstance();
    static void destroyInstance();
    
    // Configuration
    void initialize(const string& filename = "", bool console = true);
    void setLogLevel(LogLevel level);
    void finalize();
    
    // Core logging hooks (inserted at event locations)
    void logPacketCreated(int packet_id, int src, int dst, int timestamp);
    void logPacketForwarded(int packet_id, int from_node, int to_node, int next_hop, int timestamp);
    void logPacketDelivered(int packet_id, int at_node, const vector<int>& path, int timestamp);
    void logPacketDropped(int packet_id, int at_node, const string& reason, int timestamp);
    void logRoutingDecision(int packet_id, int src_node, int dst_node, int next_hop, int timestamp);
    void logQueueEvent(int packet_id, LogEventType event, int node_id, int timestamp);
    
    // Trace functions
    void recordHop(int packet_id, int node_id);
    vector<int> getPacketTrace(int packet_id) const;
    
    // Output
    void printLogs() const;
    void exportToFile();
    void exportToCSV(const string& filename);
    void exportToJSON(ostream& out) const;
    
    // Stats
    int getTotalLogs() const { return total_logs; }
};

// ============================================================================
// Simple Logging API (for general purpose debugging)
// ============================================================================
// (LogLevel enum is defined in log_types.h)

void log(LogLevel level, const std::string& msg);

#endif // NETWORK_LOGGER_H