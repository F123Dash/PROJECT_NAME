#include "metrics.h"
#include "../src/simulator_clock.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

using namespace std;

// ============================================================================
// Static Member Initialization
// ============================================================================
MetricsManager* MetricsManager::instance = nullptr;

// ============================================================================
// Singleton Implementation
// ============================================================================

MetricsManager::MetricsManager()
    : next_packet_id(0),
      sim_start_time(0),
      sim_end_time(0),
      csv_filename(""),
      csv_enabled(false) {
}

MetricsManager* MetricsManager::getInstance() {
    if (instance == nullptr) {
        instance = new MetricsManager();
    }
    return instance;
}

void MetricsManager::destroyInstance() {
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;
    }
}

// ============================================================================
// Lifecycle Methods
// ============================================================================

void MetricsManager::initialize(int start_time, const string& csv_file) {
    sim_start_time = start_time;
    next_packet_id = 0;
    packets.clear();
    
    if (!csv_file.empty()) {
        csv_filename = csv_file;
        csv_enabled = true;
        cout << "[MetricsManager] CSV export enabled: " << csv_file << endl;
    }
    
    cout << "[MetricsManager] Initialized (start_time=" << start_time << ")" << endl;
}

void MetricsManager::finalize(int end_time) {
    sim_end_time = end_time;
    cout << "[MetricsManager] Finalized (end_time=" << end_time << ")" << endl;
    cout << "[MetricsManager] Total packets tracked: " << packets.size() << endl;
}

// ============================================================================
// Hook Methods - Core Tracking
// ============================================================================

int MetricsManager::onPacketCreated(int src, int dst, int creation_time, int size) {
    int packet_id = next_packet_id++;
    
    PerPacketMetrics ppm;
    ppm.packet_id = packet_id;
    ppm.source = src;
    ppm.destination = dst;
    ppm.creation_time = creation_time;
    ppm.status = PacketStatus::CREATED;
    
    packets[packet_id] = ppm;
    
    cout << "[Metrics] CREATED packet_id=" << packet_id
         << " src=" << src << " dst=" << dst
         << " size=" << size << " time=" << creation_time << endl;
    
    return packet_id;
}

void MetricsManager::onPacketHop(int packet_id, int node_id, int current_time) {
    if (packets.find(packet_id) == packets.end()) {
        cerr << "[MetricsManager] ERROR: onPacketHop - packet " << packet_id << " not found!" << endl;
        return;
    }
    
    PerPacketMetrics& ppm = packets[packet_id];
    ppm.hop_count++;
    
    // Record first hop time
    if (ppm.first_hop_time == -1) {
        ppm.first_hop_time = current_time;
    }
    
    if (ppm.status == PacketStatus::CREATED) {
        ppm.status = PacketStatus::IN_TRANSIT;
    }
    
    cout << "[Metrics] HOP packet_id=" << packet_id
         << " at_node=" << node_id
         << " hop_count=" << ppm.hop_count << endl;
}

void MetricsManager::onPacketDelivered(int packet_id, int delivery_time, int final_hop_count) {
    if (packets.find(packet_id) == packets.end()) {
        cerr << "[MetricsManager] ERROR: onPacketDelivered - packet " << packet_id << " not found!" << endl;
        return;
    }
    
    PerPacketMetrics& ppm = packets[packet_id];
    ppm.delivery_time = delivery_time;
    ppm.latency = delivery_time - ppm.creation_time;
    ppm.hop_count = final_hop_count;
    ppm.status = PacketStatus::DELIVERED;
    ppm.status_reason = "OK";
    
    cout << "[Metrics] DELIVERED packet_id=" << packet_id
         << " latency=" << ppm.latency
         << " hops=" << final_hop_count << endl;
}

void MetricsManager::onPacketDropped_TTL(int packet_id, int current_time) {
    if (packets.find(packet_id) == packets.end()) {
        cerr << "[MetricsManager] ERROR: onPacketDropped_TTL - packet " << packet_id << " not found!" << endl;
        return;
    }
    
    PerPacketMetrics& ppm = packets[packet_id];
    ppm.delivery_time = -1;
    ppm.latency = -1;
    ppm.status = PacketStatus::DROPPED_TTL;
    ppm.status_reason = "TTL expired";
    
    cout << "[Metrics] DROPPED packet_id=" << packet_id
         << " reason=TTL_EXPIRED" << endl;
}

void MetricsManager::onPacketDropped_NoRoute(int packet_id, int current_time) {
    if (packets.find(packet_id) == packets.end()) {
        cerr << "[MetricsManager] ERROR: onPacketDropped_NoRoute - packet " << packet_id << " not found!" << endl;
        return;
    }
    
    PerPacketMetrics& ppm = packets[packet_id];
    ppm.delivery_time = -1;
    ppm.latency = -1;
    ppm.status = PacketStatus::DROPPED_NO_ROUTE;
    ppm.status_reason = "No route to destination";
    
    cout << "[Metrics] DROPPED packet_id=" << packet_id
         << " reason=NO_ROUTE" << endl;
}
void MetricsManager::onPacketDropped_QueueOverflow(int packet_id, int current_time) {
    if (packets.find(packet_id) == packets.end()) {
        cerr << "[MetricsManager] ERROR: onPacketDropped_QueueOverflow - packet " << packet_id << " not found!" << endl;
        return;
    }
    
    PerPacketMetrics& ppm = packets[packet_id];
    ppm.delivery_time = -1;
    ppm.latency = -1;
    ppm.status = PacketStatus::DROPPED_OTHER;
    ppm.status_reason = "Queue overflow";
    
    cout << "[Metrics] DROPPED packet_id=" << packet_id
         << " reason=QUEUE_OVERFLOW" << endl;
}

void MetricsManager::onQueueDelay(int packet_id, int delay_ms) {
    if (packets.find(packet_id) == packets.end()) {
        cerr << "[MetricsManager] ERROR: onQueueDelay - packet " << packet_id << " not found!" << endl;
        return;
    }
    
    PerPacketMetrics& ppm = packets[packet_id];
    ppm.queue_delay = delay_ms;
    
    cout << "[Metrics] QUEUE_DELAY packet_id=" << packet_id
         << " delay=" << delay_ms << " ms" << endl;
}
// ============================================================================
// Query Methods
// ============================================================================

const PerPacketMetrics* MetricsManager::getPacketMetrics(int packet_id) const {
    auto it = packets.find(packet_id);
    if (it != packets.end()) {
        return &(it->second);
    }
    return nullptr;
}

// ============================================================================
// Metrics Computation - Efficient Aggregation
// ============================================================================

GlobalMetrics MetricsManager::computeGlobalMetrics() {
    GlobalMetrics gm;
    
    if (packets.empty()) {
        cout << "[MetricsManager] Warning: No packets tracked!" << endl;
        return gm;
    }
    
    gm.total_packets = packets.size();
    
    vector<int> latencies;
    int total_hops = 0;
    
    // Single pass through all packets
    for (const auto& [id, ppm] : packets) {
        if (ppm.status == PacketStatus::DELIVERED) {
            gm.delivered++;
            latencies.push_back(ppm.latency);
            total_hops += ppm.hop_count;
            
            gm.min_latency = min(gm.min_latency, ppm.latency);
            gm.max_latency = max(gm.max_latency, ppm.latency);
        } else {
            gm.dropped++;
            if (ppm.status == PacketStatus::DROPPED_TTL) {
                gm.dropped_ttl++;
            } else if (ppm.status == PacketStatus::DROPPED_NO_ROUTE) {
                gm.dropped_no_route++;
            }
        }
    }
    
    // Calculate averages
    if (!latencies.empty()) {
        double sum = 0.0;
        for (int lat : latencies) {
            sum += lat;
        }
        gm.avg_latency = sum / latencies.size();
        
        // Calculate jitter (standard deviation)
        double variance = 0.0;
        for (int lat : latencies) {
            variance += (lat - gm.avg_latency) * (lat - gm.avg_latency);
        }
        gm.jitter = sqrt(variance / latencies.size());
    }
    
    if (gm.delivered > 0) {
        gm.avg_hops = (double)total_hops / gm.delivered;
    }
    
    // Calculate loss rate
    gm.loss_rate = (gm.total_packets > 0)
        ? (double)gm.dropped / gm.total_packets * 100.0
        : 0.0;
    
    // Calculate throughput (packets per time unit)
    int time_range = sim_end_time - sim_start_time;
    gm.throughput = (time_range > 0)
        ? (double)gm.delivered / time_range
        : 0.0;
    
    return gm;
}

// ============================================================================
// Output Methods
// ============================================================================

void MetricsManager::printMetricsReport() {
    GlobalMetrics gm = computeGlobalMetrics();
    
    cout << "\n";
    cout << "=========================================================" << endl;
    cout << "             SIMULATION METRICS REPORT" << endl;
    cout << "=========================================================" << endl;
    
    cout << "\n--- PACKET STATISTICS ---" << endl;
    cout << "Total Packets:                " << gm.total_packets << endl;
    cout << "Successfully Delivered:       " << gm.delivered << endl;
    cout << "Dropped:                      " << gm.dropped << endl;
    cout << "  - TTL Expired:              " << gm.dropped_ttl << endl;
    cout << "  - No Route:                 " << gm.dropped_no_route << endl;
    
    cout << "\n--- LATENCY METRICS (ms) ---" << endl;
    cout << fixed << setprecision(3);
    cout << "Average Latency:              " << gm.avg_latency << " ms" << endl;
    cout << "Minimum Latency:              " << (gm.min_latency == INT_MAX ? 0 : gm.min_latency) << " ms" << endl;
    cout << "Maximum Latency:              " << gm.max_latency << " ms" << endl;
    cout << "Jitter (Std. Dev):            " << gm.jitter << " ms" << endl;
    
    cout << "\n--- NETWORK PERFORMANCE ---" << endl;
    cout << "Packet Loss Rate:             " << gm.loss_rate << " %" << endl;
    cout << "Average Hops per Packet:      " << gm.avg_hops << endl;
    cout << "Throughput (pkt/ms):          " << gm.throughput << endl;
    
    cout << "\n--- SIMULATION TIME ---" << endl;
    cout << "Start Time:                   " << sim_start_time << " ms" << endl;
    cout << "End Time:                     " << sim_end_time << " ms" << endl;
    cout << "Duration:                     " << (sim_end_time - sim_start_time) << " ms" << endl;
    
    cout << "\n=========================================================" << endl;
    cout << endl;
}

void MetricsManager::exportMetricsCSV() {
    if (csv_filename.empty()) {
        cout << "[MetricsManager] Warning: CSV filename not set" << endl;
        return;
    }
    exportMetricsToFile(csv_filename);
}

void MetricsManager::exportMetricsToFile(const string& filename) {
    ofstream file(filename);
    
    if (!file.is_open()) {
        cerr << "[MetricsManager] ERROR: Could not open " << filename << endl;
        return;
    }
    
    // Header
    file << "PacketID,Source,Destination,CreationTime,DeliveryTime,Latency,"
         << "HopCount,Status,StatusReason\n";
    
    // Per-packet data
    for (const auto& [id, ppm] : packets) {
        string status_str;
        if (ppm.status == PacketStatus::DELIVERED) status_str = "DELIVERED";
        else if (ppm.status == PacketStatus::DROPPED_TTL) status_str = "DROPPED_TTL";
        else if (ppm.status == PacketStatus::DROPPED_NO_ROUTE) status_str = "DROPPED_NO_ROUTE";
        else status_str = "DROPPED_OTHER";
        
        file << ppm.packet_id << ","
             << ppm.source << ","
             << ppm.destination << ","
             << ppm.creation_time << ","
             << ppm.delivery_time << ","
             << ppm.latency << ","
             << ppm.hop_count << ","
             << status_str << ","
             << ppm.status_reason << "\n";
    }
    
    // Global metrics
    GlobalMetrics gm = computeGlobalMetrics();
    
    file << "\n--- GLOBAL METRICS ---\n";
    file << "Metric,Value\n";
    file << "Total Packets," << gm.total_packets << "\n";
    file << "Delivered," << gm.delivered << "\n";
    file << "Dropped," << gm.dropped << "\n";
    file << "Average Latency (ms)," << gm.avg_latency << "\n";
    file << "Jitter (ms)," << gm.jitter << "\n";
    file << "Min Latency (ms)," << (gm.min_latency == INT_MAX ? 0 : gm.min_latency) << "\n";
    file << "Max Latency (ms)," << gm.max_latency << "\n";
    file << "Packet Loss Rate (%)," << gm.loss_rate << "\n";
    file << "Average Hops," << gm.avg_hops << "\n";
    file << "Throughput (pkt/ms)," << gm.throughput << "\n";
    
    file.close();
    cout << "[MetricsManager] Metrics exported to " << filename << endl;
}
