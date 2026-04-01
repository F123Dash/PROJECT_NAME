#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <cstdint>
#include <ctime>

// Extended Packet definition for transport layer
struct Packet {
    // Core network fields
    int source;
    int destination;
    int protocol;  // 0=TCP, 1=UDP
    
    // Transport layer fields (REQUIRED for TCP/UDP)
    uint32_t sequence_number;
    uint32_t ack_number;
    bool is_ack;
    int flow_id;  // identifies connection/flow
    int payload_size;
    
    // Metadata
    double timestamp;
    
    // Constructor
    Packet() 
        : source(-1), destination(-1), protocol(-1),
          sequence_number(0), ack_number(0), is_ack(false),
          flow_id(-1), payload_size(0), timestamp(0.0) {}
    
    Packet(int src, int dst, int proto, int size)
        : source(src), destination(dst), protocol(proto),
          sequence_number(0), ack_number(0), is_ack(false),
          flow_id(-1), payload_size(size), timestamp(0.0) {}
};

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
