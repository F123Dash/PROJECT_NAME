#include "event.h"
#include <queue>
#include <vector>
#include <iostream>

using namespace std;

class EventQueue {
private:
    priority_queue<Event, vector<Event>, greater<Event>> pq;

public:
    void push(Event e) {
        pq.push(e);
    }

    Event pop() {
        Event e = pq.top();
        pq.pop();
        return e;
    }

    bool empty() {
        return pq.empty();
    }
};

// Global instances
double current_time = 0.0;
EventQueue event_queue;

// Schedule an event in the global queue
void schedule_event(Event e) {
    event_queue.push(e);
}

// Run the simulation event loop until end_time
void run_event_loop(double end_time) {
    while (!event_queue.empty()) {
        Event e = event_queue.pop();

        if (e.time > end_time)
            break;

        current_time = e.time;

        // Execute the event callback
        if (e.callback) {
            e.callback();
        }
    }
}
