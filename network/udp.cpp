#include "udp.h"
#include <iostream>

void UDP::send(int src, int dst, int size) {
    // Create UDP packet
    Packet pkt(src, dst, size, "UDP", 0, 64);
    
    // Log the send action (simulation engine will schedule actual send)
    std::cout << "[UDP] SEND: src=" << src 
              << " dst=" << dst 
              << " size=" << size << std::endl;
    
    // Return packet to simulation engine for scheduling
    // (For now, just log. Integration point for event scheduler)
}

void UDP::on_receive(const Packet &pkt) {
    // Fire-and-forget: just deliver to application layer
    std::cout << "[UDP] RECEIVE: src=" << pkt.source 
              << " dst=" << pkt.destination 
              << " size=" << pkt.size
              << " timestamp=" << pkt.timestamp << std::endl;
    
    // Application layer would process the packet here
}
