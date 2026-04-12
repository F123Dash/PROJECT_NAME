#ifndef NETWORK_LAYER_H
#define NETWORK_LAYER_H

#include "../engine/packet.h"
#include <map>

// Forward declaration
struct Node;

class NetworkLayer {
public:
    Node* node;
    std::map<int, int> retransmit_count;  // packet_id -> retry count

    explicit NetworkLayer(Node* n);
    virtual ~NetworkLayer() = default;

    // Called by Transport Layer (sending side)
    void send(Packet p);

    // Called when packet arrives at node (receiving side)
    void receive(Packet p);
    
    // Schedule packet arrival (delayed delivery)
    void schedulePacketArrival(Packet p, int next_hop, double travel_time);
    
    // Retransmission support
    void scheduleRetransmissionTimeout(Packet p, double timeout_ms);
    void handleRetransmissionTimeout(Packet p);

private:
    // Core forwarding logic
    void forward(Packet p);
};

#endif
