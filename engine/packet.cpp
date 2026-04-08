#include "packet.h"
#include <iostream>
using namespace std;

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
