#ifndef PACKET_FLOW_H
#define PACKET_FLOW_H

#include "../engine/packet.h"

// Core packet movement handlers
void handle_packet_forward(Packet p, int current_node, int next_hop);
void handle_packet_dequeue(Packet p, int current_node, int next_hop);

#endif
