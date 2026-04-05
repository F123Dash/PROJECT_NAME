#ifndef FLOW_H
#define FLOW_H

enum class TrafficType {
    CONSTANT,   // Constant bitrate (CBR)
    RANDOM,     // Poisson (exponential inter-arrival)
    BURST       // Burst + silence alternation
};

struct Flow {
    int source;
    int destination;

    double rate;            // packets per second
    int packet_size;        // bytes

    double start_time;      // when flow begins
    double duration;        // how long flow lasts

    TrafficType type;

    // burst-specific parameters
    double burst_rate;      // high rate during burst
    double burst_duration;  // silence duration between bursts

    // protocol selection
    bool use_tcp;           // true for TCP, false for UDP
};

#endif
