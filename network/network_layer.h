#ifndef NETWORK_LAYER_H
#define NETWORK_LAYER_H

#include "../engine/packet.h"

// Forward declaration
struct Node;

class NetworkLayer {
public:
    Node* node;

    explicit NetworkLayer(Node* n);
    virtual ~NetworkLayer() = default;

    // Called by Transport Layer (sending side)
    void send(Packet p);

    // Called when packet arrives at node (receiving side)
    void receive(Packet p);

private:
    // Core forwarding logic
    void forward(Packet p);
};

#endif
