#include "congestion.h"
#include <iostream>
#include <cmath>
#include <iomanip>

using namespace std;

CongestionManager* CongestionManager::instance = nullptr;

CongestionManager::CongestionManager()
    : base_delay(1.0),
      congestion_alpha(0.5),
      queue_aware(true),
      debug_enabled(false) {
}

CongestionManager* CongestionManager::getInstance() {
    if (instance == nullptr) {
        instance = new CongestionManager();
    }
    return instance;
}

void CongestionManager::destroyInstance() {
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;
    }
}

void CongestionManager::setBaseDelay(double delay_ms) {
    if (delay_ms > 0) {
        base_delay = delay_ms;
    }
}

void CongestionManager::setCongestionAlpha(double alpha) {
    if (alpha >= 0.0 && alpha <= 1.0) {
        congestion_alpha = alpha;
    }
}

void CongestionManager::updateCongestion(int node_id, int queue_size, int queue_capacity) {
    if (queue_capacity <= 0) {
        node_congestion[node_id] = 0.0;
        return;
    }
    
    // Congestion level: queue utilization (0.0 to 1.0)
    double utilization = (double)queue_size / queue_capacity;
    utilization = min(1.0, utilization);  // Cap at 1.0
    
    node_congestion[node_id] = utilization;
    
    if (debug_enabled && utilization > 0.5) {
        cout << "[Congestion] Node " << node_id
             << " congestion=" << fixed << setprecision(2) << utilization
             << " (queue " << queue_size << "/" << queue_capacity << ")" << endl;
    }
}

double CongestionManager::getDelay(int node_id, int queue_size) const {
    auto it = node_congestion.find(node_id);
    double congestion_level = (it != node_congestion.end()) ? it->second : 0.0;
    
    // Delay formula: base_delay + alpha * congestion_level * base_delay
    // This makes delay increase proportionally with congestion
    double delay = base_delay * (1.0 + congestion_alpha * congestion_level);
    
    return delay;
}

double CongestionManager::getCongestionLevel(int node_id) const {
    auto it = node_congestion.find(node_id);
    return (it != node_congestion.end()) ? it->second : 0.0;
}

bool CongestionManager::shouldDropPacket(int node_id, double drop_threshold) const {
    double congestion = getCongestionLevel(node_id);
    return congestion >= drop_threshold;
}

void CongestionManager::clearCongestion() {
    node_congestion.clear();
}

void CongestionManager::printCongestionStats() const {
    cout << "\n--- Congestion Statistics ---" << endl;
    cout << "Base delay: " << fixed << setprecision(2) << base_delay << " ms" << endl;
    cout << "Congestion alpha: " << congestion_alpha << endl;
    cout << "Nodes with congestion data: " << node_congestion.size() << endl;
    
    double avg_congestion = 0.0;
    double max_congestion = 0.0;
    
    for (const auto& [node_id, level] : node_congestion) {
        avg_congestion += level;
        max_congestion = max(max_congestion, level);
    }
    
    if (!node_congestion.empty()) {
        avg_congestion /= node_congestion.size();
        cout << "Average congestion: " << fixed << setprecision(3) << avg_congestion << endl;
        cout << "Max congestion: " << max_congestion << endl;
    }
}
