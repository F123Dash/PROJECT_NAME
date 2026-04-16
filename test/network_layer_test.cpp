#include <iostream>
#include <map>
#include <memory>
#include "../engine/node.h"
#include "../engine/packet.h"
#include "../network/network_layer.h"

using namespace std;

int main() {
    cout << "[TEST] Network Layer Tests\n\n";

    // Create test nodes using unique_ptr to avoid default constructor
    map<int, unique_ptr<Node>> nodes;
    for (int i = 0; i < 5; i++) {
        nodes[i] = make_unique<Node>(i);
        nodes[i]->initialize_network_layer();
    }

    // Test 1: Packet with TTL = 1 forwarded once (should expire)
    Packet p1(0, 4, 512, "UDP", 0, 1);
    cout << "[Test 1] Packet TTL=1 - initial ttl: " << p1.ttl << "\n";
    // BUG: If forwarded, TTL becomes 0, packet should drop but code might not check properly
    if (p1.ttl <= 0) {
        cout << "  Already expired before forwarding!\n";
    }

    // Test 2: Missing routing entry
    Packet p2(0, 3, 256, "TCP", 0, 64);
    nodes[0]->add_route(1, 2);  // Only route to node 1
    // BUG: No route to destination 3, should drop
    int next_hop = nodes[0]->get_next_hop(3);
    cout << "[Test 2] No route to destination 3 - next_hop: " << next_hop 
         << " (SHOULD BE -1)\n";

    // Test 3: Routing table points to invalid next hop
    Packet p3(0, 2, 128, "UDP", 0, 64);
    nodes[0]->add_route(2, 99);  // Route to non-existent node 99
    // BUG: next_hop doesn't validate if node exists in network
    next_hop = nodes[0]->get_next_hop(2);
    cout << "[Test 3] Route to invalid node 99 - next_hop: " << next_hop 
         << " (NODE DOESN'T EXIST)\n";

    // Test 4: Packet with negative TTL
    Packet p4(1, 3, 512, "TCP", 0, -5);  // BUG: Negative TTL
    cout << "[Test 4] Packet TTL<0 - ttl: " << p4.ttl 
         << " (SHOULD BE >= 0)\n";

    // Test 5: Source and destination are same
    Packet p5(2, 2, 256, "UDP", 0, 64);  // BUG: Self-destined packet
    cout << "[Test 5] Self-destined packet - src: " << p5.source 
         << " dst: " << p5.destination << "\n";

    // Test 6: Cyclic routing (A->B, B->A)
    nodes[0]->add_route(1, 1);  // Node 0: to reach node 1, go to node 1
    nodes[1]->add_route(0, 0);  // Node 1: to reach node 0, go to node 0 (cycle!)
    // BUG: Packet bounces between nodes indefinitely (until TTL expires)
    cout << "[Test 6] Cyclic route detected: 0->1, 1->0\n";

    // Test 7: Path history grows without bound
    Packet p7(0, 4, 512, "TCP", 0, 64);
    for (int i = 0; i < 100; i++) {
        p7.record_hop(i);  // BUG: path_history can grow very large
    }
    cout << "[Test 7] Path history size: " << p7.path_history.size() 
         << " (NO LIMIT!)\n";

    // Test 8: Empty routing table
    Packet p8(2, 3, 128, "UDP", 0, 64);
    // Node 2 has empty routing table (never initialized routes)
    next_hop = nodes[2]->get_next_hop(3);
    cout << "[Test 8] Query empty routing table - next_hop: " << next_hop 
         << "\n";

    cout << "\nAll network layer tests completed\n";
    return 0;
}
