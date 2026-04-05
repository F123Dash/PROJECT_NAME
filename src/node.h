#ifndef NODE_H
#define NODE_H

#include <map>
#include <queue>
#include <vector>
#include <string>
#include "packet.h"
using namespace std;

// Forward declaration
class NetworkLayer;

struct Node {
    int id;                          // unique node ID
    map<int, int> routing_table;     // destination → next hop node ID
    queue<Packet> pkt_queue;         // packets waiting to be sent
    vector<int> interfaces;          // connected link IDs
    NetworkLayer* network_layer;     // network layer for routing and forwarding

    // constructor
    Node(int node_id);
    
    // Initialize network layer (call after construction)
    void initialize_network_layer();

    void add_route(int destination, int next_hop);

    void add_interface(int link_id);     // add a link interface

    int get_next_hop(int destination) const; // (-1 if no route)

    void send_packet(Packet& pkt); //puts packet in queue, records hop

    void receive_packet(Packet& pkt); //processes incoming packet

    // debug
    void print_info() const;
};

#endif
