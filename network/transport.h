#ifndef TRANSPORT_H
#define TRANSPORT_H

#include "../engine/packet.h"

// Base Transport interface
class Transport {
public:
    virtual ~Transport() = default;
    
    // Send data from src to dst
    virtual void send(int src, int dst, int size) = 0;
    
    // Process received packet
    virtual void on_receive(const Packet &pkt) = 0;
};

#endif  // TRANSPORT_H
