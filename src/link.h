#ifndef LINK_H
#define LINK_H

#include <string>
#include "packet.h"
using namespace std;

struct Link {
    int id;
    int bandwidth;
    int delay;
    double loss_probability;
    int queue_capacity;
    int node1;
    int node2;
    bool is_up;

    Link(int link_id, int bw, int dl, double loss, int capacity, int n1, int n2);

    bool packet_lost();
    int transmission_delay(Packet& pkt);
    bool is_available();
    void print_info();
};

#endif
