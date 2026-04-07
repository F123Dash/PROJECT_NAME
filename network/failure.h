#ifndef NETWORK_FAILURE_H
#define NETWORK_FAILURE_H

#include <map>
#include <set>
#include <utility>

using namespace std;

// Link status (bidirectional link: (u,v) or (v,u) both represent same link)
enum class LinkStatus {
    ACTIVE,
    DOWN
};

// Manages link state (failures, up/down transitions)
class FailureManager {
private:
    static FailureManager* instance;
    
    // Link state: (node_a, node_b) → status
    // Stores links as sorted pairs: min(a,b), max(a,b) for uniqueness
    map<pair<int,int>, LinkStatus> link_states;
    
    // Failed links count
    int total_failures;
    bool debug_enabled;
    
    FailureManager();
    pair<int,int> normalize_link(int u, int v) const;
    
public:
    static FailureManager* getInstance();
    static void destroyInstance();
    
    // Configuration
    void enableDebug(bool enable) { debug_enabled = enable; }
    
    // Link state management
    void activateLink(int u, int v);
    void deactivateLink(int u, int v);
    bool isLinkActive(int u, int v) const;
    
    // Mass operations
    void activateAllLinks();
    void simulateRandomFailures(int num_failures);
    
    // Stats
    int getTotalFailures() const { return total_failures; }
    void printLinkStatus() const;
};

#endif // NETWORK_FAILURE_H
