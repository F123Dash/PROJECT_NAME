#include "failure.h"
#include <iostream>
#include <random>
#include <algorithm>

using namespace std;

FailureManager* FailureManager::instance = nullptr;

FailureManager::FailureManager()
    : total_failures(0), debug_enabled(false) {
}

FailureManager* FailureManager::getInstance() {
    if (instance == nullptr) {
        instance = new FailureManager();
    }
    return instance;
}

void FailureManager::destroyInstance() {
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;
    }
}

pair<int,int> FailureManager::normalize_link(int u, int v) const {
    return (u < v) ? make_pair(u, v) : make_pair(v, u);
}

void FailureManager::activateLink(int u, int v) {
    auto link = normalize_link(u, v);
    link_states[link] = LinkStatus::ACTIVE;
    
    if (debug_enabled) {
        cout << "[Failure] Link " << u << "-" << v << " ACTIVATED" << endl;
    }
}

void FailureManager::deactivateLink(int u, int v) {
    auto link = normalize_link(u, v);
    link_states[link] = LinkStatus::DOWN;
    total_failures++;
    
    cout << "[Failure] Link " << u << "-" << v << " FAILED" << endl;
}

bool FailureManager::isLinkActive(int u, int v) const {
    auto link = normalize_link(u, v);
    auto it = link_states.find(link);
    
    if (it == link_states.end()) {
        return true;  // Unknown link defaults to active
    }
    
    return it->second == LinkStatus::ACTIVE;
}

void FailureManager::activateAllLinks() {
    for (auto& [link, status] : link_states) {
        link_states[link] = LinkStatus::ACTIVE;
    }
    cout << "[Failure] All " << link_states.size() << " links activated" << endl;
}

void FailureManager::simulateRandomFailures(int num_failures) {
    if (link_states.empty()) {
        cout << "[Failure] Warning: No links registered" << endl;
        return;
    }
    
    vector<pair<int,int>> all_links;
    for (const auto& [link, status] : link_states) {
        all_links.push_back(link);
    }
    
    random_device rd;
    mt19937 gen(rd());
    shuffle(all_links.begin(), all_links.end(), gen);
    
    int failures_applied = 0;
    for (const auto& link : all_links) {
        if (failures_applied >= num_failures) break;
        
        link_states[link] = LinkStatus::DOWN;
        total_failures++;
        failures_applied++;
        
        cout << "[Failure] Random failure: link " << link.first << "-" 
             << link.second << " DOWN" << endl;
    }
}

void FailureManager::printLinkStatus() const {
    cout << "\n--- Link Status ---" << endl;
    cout << "Total links: " << link_states.size() << endl;
    cout << "Total failures: " << total_failures << endl;
    
    int active = 0, down = 0;
    for (const auto& [link, status] : link_states) {
        if (status == LinkStatus::ACTIVE) {
            active++;
        } else {
            down++;
            cout << "  DOWN: " << link.first << " - " << link.second << endl;
        }
    }
    
    cout << "Active: " << active << " | Down: " << down << endl;
}
