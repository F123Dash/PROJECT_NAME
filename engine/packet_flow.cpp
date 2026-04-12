#include "packet_flow.h"
#include "../engine/event.h"
#include "../engine/event_queue.h"
#include "../network/queue.h"
#include "../network/metrics.h"
#include "../engine/integration.h"
#include "../engine/node.h"

#include <iostream>
#include <algorithm>

using namespace std;

// External simulation globals
extern Integration* GLOBAL_INTEGRATION;

// =====================================================
//   HANDLE FORWARD (enqueue + delay scheduling)
// =====================================================
void handle_packet_forward(Packet p, int current_node, int next_hop) {

    //   Enqueue packet
    bool success = Queue::getInstance()->enqueue(
        current_node,
        0,
        p.source,
        p.destination,
        (int)current_time
    );

    if (!success) {
        cout << "[DROP] Queue full at node " << current_node << endl;

        MetricsManager::getInstance()->onPacketDropped_QueueOverflow(
            0, (int)current_time
        );
        return;
    }

    cout << "[QUEUE] Packet " << 0
         << " at node " << current_node << endl;

    //   Simulate link delay
    double delay = 1 + rand() % 3;

    Event e;
    e.time = current_time + delay;
    e.type = "PACKET_DEQUEUE";

    e.callback = [p, current_node, next_hop]() {
        handle_packet_dequeue(p, current_node, next_hop);
    };

    schedule_event(e);
}

// =====================================================
//   HANDLE DEQUEUE (queue → next node)
// =====================================================
void handle_packet_dequeue(Packet p, int current_node, int next_hop) {

    int pkt_id = Queue::getInstance()->dequeue(
        current_node,
        (int)current_time
    );

    if (pkt_id == -1) return;

    cout << "[DEQUEUE] Packet " << pkt_id
         << " from " << current_node
         << " → " << next_hop << endl;

    Node* next_node = GLOBAL_INTEGRATION->get_node(next_hop);
    if (!next_node) return;

    //   Schedule arrival event
    Event e;
    e.time = current_time;
    e.type = "PACKET_RECEIVE";

    e.callback = [=]() mutable {
        next_node->receive_packet(p);
    };

    schedule_event(e);
}
