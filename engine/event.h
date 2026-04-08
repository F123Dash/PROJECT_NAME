#ifndef EVENT_H
#define EVENT_H

#include <functional>
#include <string>
using namespace std;

struct Event {
    double time;               // event time (use double for precision)
    string type;              // event type (e.g., PACKET_SEND)
    function<void()> callback; // function to execute

    // for priority queue (min-heap)
    bool operator>(const Event& other) const {
        return time > other.time;
    }
};

#endif
