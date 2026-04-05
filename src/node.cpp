#include "node.h"
#include "../network/network_layer.h"
#include <iostream>
using namespace std;

Node::Node(int node_id) : network_layer(nullptr) {
    id = node_id;
}

void Node::initialize_network_layer() {
    if (!network_layer) {
        network_layer = new NetworkLayer(this);
        cout << "[Node " << id << "] Network layer initialized" << endl;
    }
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
    // Delegate to network layer if available
    if (network_layer) {
        network_layer->send(pkt);
    } else {
        // Fallback: legacy behavior if network layer not initialized
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
        cout << "[Node " << id << "] Packet queued -> next hop: " << next_hop << endl;
    }
}

void Node::receive_packet(Packet& pkt) {
    // Delegate to network layer if available
    if (network_layer) {
        network_layer->receive(pkt);
    } else {
        // Fallback: legacy behavior if network layer not initialized
        if (pkt.is_expired()) {
            cout << "[Node " << id << "] Packet expired! Dropping." << endl;
            return;
        }

        pkt.record_hop(id);

        if (pkt.destination == id) {
            cout << "[Node " << id << "] Packet DELIVERED!" << endl;
            pkt.print_info();
            return;
        }

        cout << "[Node " << id << "] Packet received, forwarding..." << endl;
        send_packet(pkt);
    }
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
