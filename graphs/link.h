#ifndef LINK_H
#define LINK_H

// Link properties for realistic packet transmission
struct Link {
    int from;           // source node
    int to;             // destination node
    double bandwidth;   // Mbps
    double latency;     // milliseconds (propagation delay)

    // Default constructor
    Link() : from(0), to(0), bandwidth(100), latency(2) {}

    // Parameterized constructor
    Link(int f, int t, double bw = 100, double lat = 2)
        : from(f), to(t), bandwidth(bw), latency(lat) {}

    // Calculate transmission time in milliseconds
    // transmission_time = (packet_size_bits / bandwidth_bps) * 1000
    double calculateTransmissionTime(int packet_size_bytes) const {
        if (bandwidth <= 0) return 0;
        int packet_size_bits = packet_size_bytes * 8;
        double bandwidth_bps = bandwidth * 1e6;  // Convert Mbps to bps
        double transmission_ms = (packet_size_bits / bandwidth_bps) * 1000;
        return transmission_ms;
    }

    // Total travel time = propagation delay + transmission time
    double calculateTravelTime(int packet_size_bytes) const {
        return latency + calculateTransmissionTime(packet_size_bytes);
    }
};

#endif
