#ifndef NETWORK_CONGESTION_H
#define NETWORK_CONGESTION_H

#include <map>

using namespace std;

// Manages congestion state and delay computation
class CongestionManager {
private:
    static CongestionManager* instance;
    
    // Per-node congestion: node_id → (load factor 0.0-1.0)
    map<int, double> node_congestion;
    
    // Congestion parameters
    double base_delay;      // Minimum transmission delay (ms)
    double congestion_alpha;  // Weight factor for queue-based delay
    bool queue_aware;       // Use queue depth for delay calculation
    bool debug_enabled;
    
    CongestionManager();
    
public:
    static CongestionManager* getInstance();
    static void destroyInstance();
    
    // Configuration
    void setBaseDelay(double delay_ms);
    void setCongestionAlpha(double alpha);
    void setQueueAware(bool enabled) { queue_aware = enabled; }
    void enableDebug(bool enable) { debug_enabled = enable; }
    
    // Congestion computation
    void updateCongestion(int node_id, int queue_size, int queue_capacity);
    double getDelay(int node_id, int queue_size = 0) const;
    double getCongestionLevel(int node_id) const;
    
    // Packet drop probability (optional)
    bool shouldDropPacket(int node_id, double drop_threshold = 0.8) const;
    
    // Reset
    void clearCongestion();
    
    // Stats
    void printCongestionStats() const;
};

#endif // NETWORK_CONGESTION_H
