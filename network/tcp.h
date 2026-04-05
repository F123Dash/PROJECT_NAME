#ifndef TCP_H
#define TCP_H

#include "transport.h"
#include <map>
#include <cstdint>

class TCP : public Transport {
private:
    // Congestion control state
    int cwnd;           // congestion window (packets allowed in flight)
    int ssthresh;       // slow start threshold
    
    // Sequence tracking
    uint32_t next_seq;      // next sequence number to use
    uint32_t expected_ack;  // next ACK we expect to receive
    
    // Buffering for reliability
    std::map<uint32_t, Packet> unacked_packets;  // storage for retransmission
    
    // Connection state
    int flow_id;
    bool is_congestion_avoidance;  // mode flag: false=slow start, true=congestion avoidance
    
public:
    TCP();
    virtual ~TCP() = default;
    
    // Transport interface
    void send(int src, int dst, int size) override;
    void on_receive(const Packet &pkt) override;
    
    // TCP-specific methods
    void on_ack(uint32_t ack_num);
    void retransmit(uint32_t seq);
    
    // Query state (for debugging/monitoring)
    int get_cwnd() const { return cwnd; }
    int get_ssthresh() const { return ssthresh; }
    uint32_t get_next_seq() const { return next_seq; }
    int get_unacked_count() const { return unacked_packets.size(); }
};

#endif  // TCP_H
