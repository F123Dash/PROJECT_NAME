#ifndef UDP_H
#define UDP_H

#include "transport.h"
#include <iostream>

class UDP : public Transport {
public:
    UDP() = default;
    virtual ~UDP() = default;
    
    // Send UDP packet
    void send(int src, int dst, int size) override;
    
    // Receive UDP packet
    void on_receive(const Packet &pkt) override;
};

#endif  // UDP_H
