#ifndef NODE_H
#define NODE_H

#include <map>
#include <queue>
#include <vector>
#include <string>
#include "packet.h"
using namespace std;

// Forward declarations
class NetworkLayer;
class Transport;
class Graph;

struct Node {
    int id;                          // unique node ID
    vector<int> routing_table;       // destination → next hop node ID (indexed by dest)
    queue<Packet> pkt_queue;         // packets waiting to be sent
    vector<int> interfaces;          // connected link IDs
    
    // Core components
    NetworkLayer* network_layer;     // network layer for routing and forwarding
    Transport* transport;            // transport protocol (TCP/UDP)
    Graph* graph;                    // reference to network topology

    // constructor
    Node(int node_id = 0);
    
    // Destructor
    virtual ~Node() = default;
    
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
