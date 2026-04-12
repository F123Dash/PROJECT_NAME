#include "event.h"
#include <queue>
#include <vector>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <thread>

using namespace std;

// Global event queue - all events scheduled through schedule_event()
double current_time = 0.0;
vector<Event> event_queue;  // Using single vector for all events

// Real-time control flags
bool REAL_TIME_MODE = false;
double TIME_SCALE = 1.0;
int MAX_SLEEP_MS = 50;

// Retransmission control flags
bool ENABLE_RETRANSMISSION = false;
double RTX_TIMEOUT_MS = 50.0;
int MAX_RETRANSMISSIONS = 3;

// Schedule an event in the global queue
void schedule_event(Event e) {
    event_queue.push_back(e);
    push_heap(event_queue.begin(), event_queue.end(), greater<Event>());
}

// Run the simulation event loop until end_time
void run_event_loop(double end_time) {
    using namespace std::chrono;

    auto real_start = high_resolution_clock::now();
    double sim_start = 0.0;

    while (!event_queue.empty()) {
        // Get the event with earliest time
        pop_heap(event_queue.begin(), event_queue.end(), greater<Event>());
        Event e = event_queue.back();
        event_queue.pop_back();

        if (e.time > end_time)
            break;

        // ================ REAL-TIME SYNCHRONIZATION ================
        if (REAL_TIME_MODE) {
            double sim_elapsed = e.time - sim_start;

            // Calculate expected real time for this simulation time
            auto expected_real = real_start + duration<double>(sim_elapsed / TIME_SCALE);

            auto now = high_resolution_clock::now();

            // If we're ahead of schedule, sleep to maintain real-time sync
            if (expected_real > now) {
                auto sleep_time = expected_real - now;

                // Cap sleep to MAX_SLEEP_MS to avoid long blocking
                if (sleep_time > milliseconds(MAX_SLEEP_MS)) {
                    sleep_time = milliseconds(MAX_SLEEP_MS);
                }

                std::this_thread::sleep_for(sleep_time);
            }
        }
        // ============================================================

        current_time = e.time;

        // Execute the event callback
        if (e.callback) {
            e.callback();
        }
    }
}
