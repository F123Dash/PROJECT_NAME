#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include "event.h"
#include <vector>

// Global event queue and time
extern double current_time;
extern std::vector<Event> event_queue;

// Real-time control flags
extern bool REAL_TIME_MODE;
extern double TIME_SCALE;
extern int MAX_SLEEP_MS;

// Retransmission control flags
extern bool ENABLE_RETRANSMISSION;
extern double RTX_TIMEOUT_MS;
extern int MAX_RETRANSMISSIONS;

// Functions for event scheduling
void schedule_event(Event e);
void run_event_loop(double end_time);

#endif // EVENT_QUEUE_H

