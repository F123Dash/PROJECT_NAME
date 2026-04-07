#include "packet.h"
#include <iostream>
using namespace std;

Packet::Packet(int src, int dest, int sz, string proto, int ts, int ttl_val) {
    source      = src;
    destination = dest;
    size        = sz;
    protocol    = proto;
    timestamp   = ts;
    ttl         = ttl_val;
    path_history.push_back(src); // source is the first hop
}

bool Packet::is_expired() const {
    return ttl <= 0;
}

void Packet::record_hop(int node_id) {
    path_history.push_back(node_id);
    ttl--;
}

void Packet::print_info() const {
    cout << "[Packet]"
         << " src="   << source
         << " dest="  << destination
         << " size="  << size
         << " proto=" << protocol
         << " ts="    << timestamp
         << " ttl="   << ttl
         << " path=[";
    for (int i = 0; i < path_history.size(); i++) {
        cout << path_history[i];
        if (i != path_history.size() - 1) cout << " -> ";
    }
    cout << "]" << endl;
}
