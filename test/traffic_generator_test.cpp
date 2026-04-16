#include <iostream>
#include <map>
#include <queue>
#include <vector>
#include "../network/traffic_generator.h"
#include "../engine/node.h"
#include "../engine/event_types.h"

// Global variables needed by traffic_generator.cpp
std::vector<Event> event_queue;
double current_simulation_time = 0.0;

using namespace std;

int main() {
    // Test 1: Flow with negative rate (should be positive)
    Flow f1;
    f1.source = 0;
    f1.destination = 5;
    f1.rate = -10;  // BUG: negative rate causes infinite scheduling delay
    f1.packet_size = 512;
    f1.start_time = 0;
    f1.duration = 100;
    f1.type = TrafficType::CONSTANT;
    f1.use_tcp = false;

    cout << "Flow 1 rate: " << f1.rate << "\n";

    // Test 2: Flow with start_time > start_time + duration
    Flow f2;
    f2.source = 1;
    f2.destination = 3;
    f2.rate = 5;
    f2.packet_size = 256;
    f2.start_time = 100;
    f2.duration = 50;  // BUG: duration is less than start_time offset, flow never runs
    f2.type = TrafficType::RANDOM;
    f2.use_tcp = true;

    cout << "Flow 2 start: " << f2.start_time << " duration: " << f2.duration 
         << " end: " << (f2.start_time + f2.duration) << "\n";

    // Test 3: Flow with burst_rate = 0
    Flow f3;
    f3.source = 2;
    f3.destination = 4;
    f3.rate = 20;
    f3.packet_size = 128;
    f3.start_time = 0;
    f3.duration = 50;
    f3.type = TrafficType::BURST;
    f3.burst_rate = 0;  // BUG: zero burst_rate causes division by zero (1.0 / 0)
    f3.burst_duration = 10;
    f3.use_tcp = false;

    cout << "Flow 3 burst_rate: " << f3.burst_rate << "\n";

    // Test 4: Invalid source node
    Flow f4;
    f4.source = -1;  // BUG: negative source node ID
    f4.destination = 5;
    f4.rate = 8;
    f4.packet_size = 512;
    f4.start_time = 0;
    f4.duration = 10;
    f4.type = TrafficType::CONSTANT;
    f4.use_tcp = false;

    cout << "Flow 4 source: " << f4.source << " (NEGATIVE!)\n";

    // Test 5: Zero duration flow
    Flow f5;
    f5.source = 0;
    f5.destination = 1;
    f5.rate = 50;
    f5.packet_size = 1024;
    f5.start_time = 5;
    f5.duration = 0;  // BUG: zero duration means flow generates no packets
    f5.type = TrafficType::CONSTANT;
    f5.use_tcp = false;

    cout << "Flow 5 duration: " << f5.duration << " (ZERO!)\n";

    cout << "All traffic generator tests completed\n";
    return 0;
}
