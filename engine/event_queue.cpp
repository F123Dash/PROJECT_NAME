#include "event.h"
#include <queue>
#include <vector>

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
