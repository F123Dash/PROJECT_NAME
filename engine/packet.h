#ifndef PACKET_H
#define PACKET_H

#include <string>
#include <vector>
using namespace std;

struct Packet {
    int source;              // source node ID
    int destination;         // destination node ID
    int size;                // packet size in bytes
    string protocol;         // e.g., "TCP", "UDP"
    int timestamp;           // time when packet was created
    int ttl;                 // time to live (hop count)
    vector<int> path_history; // list of node IDs the packet has visited

    // constructor
    Packet(int src, int dest, int sz, string proto, int ts, int ttl_val);

    // utility methods
    bool is_expired() const;         // returns true if TTL <= 0
    void record_hop(int node_id);    // adds node to path_history, decrements TTL
    void print_info() const;         // prints packet details (debug)
};

#endif
