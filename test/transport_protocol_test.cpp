#include <iostream>
#include "../engine/packet.h"
#include "../network/tcp.h"
#include "../network/udp.h"

using namespace std;

int main() {
    cout << "[TEST] Transport Protocol Tests\n\n";

    // Test 1: TCP without initialization
    TCP tcp_uninit;
    Packet p1(0, 1, 512, "TCP", 0, 64);
    
    // BUG: Calling send without proper state initialization
    // cwnd and ssthresh not set, may cause undefined behavior
    cout << "[Test 1] TCP send without init - cwnd: " << tcp_uninit.get_cwnd() << "\n";

    // Test 2: UDP receive with expired packet
    UDP udp;
    Packet p2(2, 3, 256, "UDP", 0, 0);  // BUG: TTL = 0 (already expired)
    p2.record_hop(2);  // This decrements TTL to -1
    p2.record_hop(3);  // TTL becomes -2
    
    cout << "[Test 2] UDP receive expired packet - TTL: " << p2.ttl 
         << " (NEGATIVE!)\n";

    // Test 3: TCP acknowledge with sequence wraparound
    TCP tcp_wrap;
    // BUG: Large ACK number without proper sequence number tracking
    uint32_t large_ack = 0xFFFFFFFF;  // Max uint32_t
    cout << "[Test 3] TCP ACK with wraparound - ack_num: " << large_ack 
         << " (NO CORRESPONDING SEQUENCE!)\n";

    // Test 4: Packet size = 0
    Packet p4(4, 5, 0, "TCP", 0, 64);  // BUG: Zero-size packet
    cout << "[Test 4] Packet with size=0 - size: " << p4.size << "\n";

    // Test 5: Source = Destination in TCP
    Packet p5(10, 10, 512, "TCP", 0, 64);  // BUG: Self-loop packet
    cout << "[Test 5] Packet with src=dst - src: " << p5.source 
         << " dst: " << p5.destination << "\n";

    // Test 6: TCP retransmit of non-existent sequence
    TCP tcp_retx;
    // BUG: Calling retransmit on sequence that was never sent
    cout << "[Test 6] TCP retransmit unacked_count: " 
         << tcp_retx.get_unacked_count() << " (NO UNACKED PACKETS)\n";

    // Test 7: UDP send receive with mismatched port (no ports in simple model)
    UDP udp_mismatch;
    Packet p7(6, 7, 128, "UDP", 0, 64);
    // BUG: No flow_id or port tracking, ambiguous which flow this belongs to
    cout << "[Test 7] UDP packet without flow identification\n";

    cout << "\nAll transport protocol tests completed\n";
    return 0;
}
