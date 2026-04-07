#ifndef NETWORK_STATE_H
#define NETWORK_STATE_H

// Optional: Unified network state management (combines failure + congestion)
// Use if you want centralized control; otherwise use FailureManager and CongestionManager separately

class NetworkState {
private:
    static NetworkState* instance;
    
    NetworkState();
    
public:
    static NetworkState* getInstance();
    static void destroyInstance();
    
    // Initialize all subsystems
    void initialize();
    
    // Get subsystem instances
    class FailureManager* getFailureManager();
    class CongestionManager* getCongestionManager();
    
    // Unified state query
    bool canTransmit(int from_node, int to_node);
    
    // Stats/reporting
    void printNetworkStatus() const;
};

#endif // NETWORK_STATE_H
