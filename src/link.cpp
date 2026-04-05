#include "link.h"
#include <iostream>
#include <cstdlib>
using namespace std;

Link::Link(int link_id, int bw, int dl, double loss, int capacity, int n1, int n2) {
    id               = link_id;
    bandwidth        = bw;
    delay            = dl;
    loss_probability = loss;
    queue_capacity   = capacity;
    node1            = n1;
    node2            = n2;
    is_up            = true;
}

bool Link::packet_lost() {
    int random = rand() % 100;
    return random < (loss_probability * 100); // fix 1: scale to 0-100
}

int Link::transmission_delay(Packet& pkt) {
    return (pkt.size * 8) / bandwidth; // fix 2: convert bytes to bits
}

bool Link::is_available() {
    return is_up;
}

void Link::print_info() {
    cout << "[Link " << id << "]"
         << " bw="       << bandwidth  << " Mbps"
         << " delay="    << delay      << " ms"
         << " loss="     << loss_probability
         << " capacity=" << queue_capacity
         << " nodes=["   << node1 << "<->" << node2 << "]"
         << " status="   << (is_up ? "UP" : "DOWN") << endl;
}
