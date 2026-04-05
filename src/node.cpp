#include "node.h"
#include <iostream>
using namespace std;

Node::Node(int node_id) {
    id = node_id;
}

void Node::add_route(int destination, int next_hop) {
    routing_table[destination] = next_hop;
}

void Node::add_interface(int link_id) {
    interfaces.push_back(link_id);
}

int Node::get_next_hop(int destination) const {
    if (routing_table.count(destination)) {
        return routing_table.at(destination);
    }
    return -1;
}

void Node::send_packet(Packet& pkt) {
    if (pkt.is_expired()) {
        cout << "[Node " << id << "] Packet expired! Dropping." << endl;
        return;
    }

    int next_hop = get_next_hop(pkt.destination);

    if (next_hop == -1) {
        cout << "[Node " << id << "] No route to " << pkt.destination << ". Dropping." << endl;
        return;
    }

    pkt_queue.push(pkt);
    // fixed: removed record_hop from here
    cout << "[Node " << id << "] Packet queued -> next hop: " << next_hop << endl;
}

void Node::receive_packet(Packet& pkt) {
    if (pkt.is_expired()) {
        cout << "[Node " << id << "] Packet expired! Dropping." << endl;
        return;
    }

    pkt.record_hop(id); // fixed: record hop when received, not when sent

    if (pkt.destination == id) {
        cout << "[Node " << id << "] Packet DELIVERED!" << endl;
        pkt.print_info();
        return;
    }

    cout << "[Node " << id << "] Packet received, forwarding..." << endl;
    send_packet(pkt);
}

void Node::print_info() const {
    cout << "[Node " << id << "]"
         << " interfaces=[";
    for (int i = 0; i < interfaces.size(); i++) {
        cout << interfaces[i];
        if (i != interfaces.size() - 1) cout << ", ";
    }
    cout << "]"
         << " routes=" << routing_table.size()
         << " queue_size=" << pkt_queue.size()
         << endl;
}
