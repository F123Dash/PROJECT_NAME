#ifndef NETWORK_LAYER_H
#define NETWORK_LAYER_H

#include "../engine/packet.h"
#include <map>
#include <set>

// Forward declaration
struct Node;

class NetworkLayer {
public:
    Node* node;
    
    // ACK tracking
    std::set<int> acknowledged_packets;     // packets that have been ACK'd
    std::map<int, int> retransmit_count;    // packet_id -> retry count
    std::map<int, int> timeout_event_ids;   // packet_id -> scheduled timeout event id

    explicit NetworkLayer(Node* n);
    virtual ~NetworkLayer() = default;

    // Called by Transport Layer (sending side)
    void send(Packet p);

    // Called when packet arrives at node (receiving side)
    void receive(Packet p);
    
    // Schedule packet arrival (delayed delivery)
    void schedulePacketArrival(Packet p, int next_hop, double travel_time, bool is_ack = false);
    
    // ACK-based retransmission support
    void scheduleRetransmissionTimeout(Packet p, double timeout_ms);
    void handleRetransmissionTimeout(Packet p);
    void sendAck(int packet_id, int source_node);
    void receiveAck(int packet_id);

private:
    // Core forwarding logic
    void forward(Packet p);
};

#endif
