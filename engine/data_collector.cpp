#include "data_collector.h"
#include <iostream>
#include <iomanip>
#include <ctime>

using namespace std;

DataCollector::DataCollector(const string& csv_filename,
                             const string& log_filename) 
    : run_id(0) {
    csv_file.open(csv_filename, ios::app);  // Append mode
    log_file.open(log_filename, ios::app);  // Append mode

    if (!csv_file.is_open()) {
        cerr << "[DataCollector] Error opening CSV file: " << csv_filename << "\n";
    }

    if (!log_file.is_open()) {
        cerr << "[DataCollector] Error opening log file: " << log_filename << "\n";
    }

    // Write CSV header if file is empty
    if (csv_file && csv_file.tellp() == 0) {
        csv_file << "run_id,timestamp,total_packets,delivered,dropped,dropped_ttl,"
                 << "dropped_no_route,avg_latency,min_latency,max_latency,jitter,"
                 << "throughput,avg_hops,loss_rate\n";
        csv_file.flush();
    }
}

DataCollector::~DataCollector() {
    if (csv_file.is_open()) {
        csv_file.flush();
        csv_file.close();
    }
    if (log_file.is_open()) {
        log_file.flush();
        log_file.close();
    }
}

void DataCollector::start_run(int id) {
    run_id = id;
    
    // Get current timestamp
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    log_file << "\n" << string(70, '=') << "\n";
    log_file << "RUN " << run_id << " - Started at " << timestamp << "\n";
    log_file << string(70, '=') << "\n";
    log_file.flush();
}

void DataCollector::record_metrics(const GlobalMetrics& gm) {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    // Write to CSV with detailed metrics
    csv_file << run_id << ","
             << timestamp << ","
             << gm.total_packets << ","
             << gm.delivered << ","
             << gm.dropped << ","
             << gm.dropped_ttl << ","
             << gm.dropped_no_route << ","
             << fixed << setprecision(2)
             << gm.avg_latency << ","
             << gm.min_latency << ","
             << gm.max_latency << ","
             << gm.jitter << ","
             << gm.throughput << ","
             << gm.avg_hops << ","
             << gm.loss_rate << "\n";
    csv_file.flush();

    // Write detailed log report
    log_file << "\n[METRICS] Run " << run_id << " - Final Results:\n";
    log_file << "  Timestamp:          " << timestamp << "\n";
    log_file << "  Total Packets:      " << gm.total_packets << "\n";
    log_file << "  Delivered:          " << gm.delivered << "\n";
    log_file << "  Dropped:            " << gm.dropped << "\n";
    log_file << "    - TTL Expired:    " << gm.dropped_ttl << "\n";
    log_file << "    - No Route:       " << gm.dropped_no_route << "\n";
    log_file << "  Loss Rate:          " << fixed << setprecision(2) << gm.loss_rate << "%\n";
    log_file << "  Avg Latency:        " << gm.avg_latency << " ms\n";
    log_file << "  Min Latency:        " << gm.min_latency << " ms\n";
    log_file << "  Max Latency:        " << gm.max_latency << " ms\n";
    log_file << "  Jitter:             " << gm.jitter << " ms\n";
    log_file << "  Throughput:         " << gm.throughput << " pkt/ms\n";
    log_file << "  Avg Hops:           " << gm.avg_hops << "\n";
    log_file << string(70, '-') << "\n";
    log_file.flush();
    latencies.push_back(gm.avg_latency);
    throughputs.push_back(gm.throughput);
    loss_rates.push_back(gm.loss_rate);
    }

void DataCollector::log_event(const string& msg) {
    log_file << "[EVENT] " << msg << "\n";
    log_file.flush();
}

void DataCollector::log_step(const string& step, const string& description) {
    log_file << "[STEP] " << step << " - " << description << "\n";
    log_file.flush();
}

void DataCollector::log_error(const string& error_msg) {
    log_file << "[ERROR] " << error_msg << "\n";
    log_file.flush();
}

void DataCollector::reset() {
    latencies.clear();
    throughputs.clear();
    loss_rates.clear();
}

const vector<double>& DataCollector::get_latencies() const {
    return latencies;
}

const vector<double>& DataCollector::get_throughputs() const {
    return throughputs;
}

const vector<double>& DataCollector::get_loss_rates() const {
    return loss_rates;
}

