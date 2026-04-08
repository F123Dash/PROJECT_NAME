#ifndef PACKET_H
#define PACKET_H

#include <string>
#include <vector>
using namespace std;

struct Packet {
    int packet_id;           // unique packet ID for metrics tracking
    int source;              // source node ID
    int destination;         // destination node ID
    int size;                // packet size in bytes
    string protocol;         // e.g., "TCP", "UDP"
    int timestamp;           // time when packet was created
    int ttl;                 // time to live (hop count)
    vector<int> path_history; // list of node IDs the packet has visited

    // constructor
    Packet(int pkt_id, int src, int dest, int sz, string proto, int ts, int ttl_val)
        : packet_id(pkt_id), source(src), destination(dest), size(sz), 
          protocol(proto), timestamp(ts), ttl(ttl_val) {}
    
    // Default constructor for legacy use
    Packet(int src, int dest, int sz, string proto, int ts, int ttl_val)
        : packet_id(-1), source(src), destination(dest), size(sz), 
          protocol(proto), timestamp(ts), ttl(ttl_val) {}

    // utility methods
    bool is_expired() const;         // returns true if TTL <= 0
    void record_hop(int node_id);    // adds node to path_history, decrements TTL
    void print_info() const;         // prints packet details (debug)
};

#endif
