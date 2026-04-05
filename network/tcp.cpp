#include "tcp.h"
#include <iostream>
#include <algorithm>

TCP::TCP() 
    : cwnd(1), ssthresh(16), next_seq(1), expected_ack(1),
      flow_id(0), is_congestion_avoidance(false) {
    std::cout << "[TCP] Initialized: cwnd=" << cwnd 
              << " ssthresh=" << ssthresh << std::endl;
}

void TCP::send(int src, int dst, int size) {
    // Step 1: Check congestion window
    if ((int)unacked_packets.size() >= cwnd) {
        std::cout << "[TCP] SEND BLOCKED: window full (unacked=" 
                  << unacked_packets.size() << " cwnd=" << cwnd << ")" << std::endl;
        return;
    }
    
    // Step 2: Create packet using canonical Packet model.
    Packet pkt(src, dst, size, "TCP", 0, 64);
    
    // Step 3: Store for retransmission
    uint32_t seq = next_seq;
    unacked_packets.insert_or_assign(seq, pkt);
    
    // Step 4: Increment sequence for next packet
    next_seq++;
    
    // Step 5: Log the send
    std::cout << "[TCP] SEND: src=" << src 
              << " dst=" << dst 
              << " size=" << size 
              << " seq=" << seq
              << " window=" << unacked_packets.size() << "/" << cwnd << std::endl;
    
    // Integration point: simulation engine will schedule timeout event
}

void TCP::on_receive(const Packet &pkt) {
    // ACK packets are represented as protocol="TCP-ACK".
    if (pkt.protocol == "TCP-ACK") {
        std::cout << "[TCP] RECEIVE ACK" << std::endl;
        on_ack(expected_ack);
    } else {
        // Data packet received
        std::cout << "[TCP] RECEIVE DATA: src=" << pkt.source 
                  << " size=" << pkt.size << std::endl;
        
        // Update expected ACK against our local sequence progression.
        expected_ack = std::max(expected_ack, next_seq);
        
        // Create and send ACK back
        Packet ack_pkt(pkt.destination, pkt.source, 0, "TCP-ACK", pkt.timestamp, 64);
        
        // Note: simulation engine will schedule this ACK send
        std::cout << "[TCP] SEND ACK: dst=" << pkt.source 
                  << " ack_state=" << expected_ack << std::endl;
    }
}

void TCP::on_ack(uint32_t ack_num) {
    // Remove all packets with seq < ack_num (cumulative ACK)
    auto it = unacked_packets.begin();
    while (it != unacked_packets.end()) {
        if (it->first < ack_num) {
            std::cout << "[TCP] ACK confirmed seq=" << it->first << std::endl;
            it = unacked_packets.erase(it);
        } else {
            ++it;
        }
    }
    
    // Update congestion control
    if (!is_congestion_avoidance) {
        // SLOW START: exponential growth
        cwnd = cwnd * 2;
        std::cout << "[TCP] SLOW START: cwnd=" << cwnd << std::endl;
        
        if (cwnd >= ssthresh) {
            is_congestion_avoidance = true;
            std::cout << "[TCP] SWITCH TO CONGESTION AVOIDANCE" << std::endl;
        }
    } else {
        // CONGESTION AVOIDANCE: linear growth
        cwnd = cwnd + 1;
        std::cout << "[TCP] CONGESTION AVOIDANCE: cwnd=" << cwnd << std::endl;
    }
}

void TCP::retransmit(uint32_t seq) {
    auto it = unacked_packets.find(seq);
    if (it == unacked_packets.end()) {
        std::cout << "[TCP] RETRANSMIT FAILED: seq=" << seq << " not found" << std::endl;
        return;
    }
    
    Packet &pkt = it->second;
    std::cout << "[TCP] RETRANSMIT: seq=" << seq 
              << " src=" << pkt.source 
              << " dst=" << pkt.destination
              << " size=" << pkt.size << std::endl;
    
    // Reduce window (congestion response)
    ssthresh = std::max(1, cwnd / 2);
    cwnd = 1;
    is_congestion_avoidance = false;  // reset to slow start
    
    std::cout << "[TCP] TIMEOUT: cwnd reset to 1, ssthresh=" << ssthresh << std::endl;
    
    // Simulation engine will reschedule this packet
}
