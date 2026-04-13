#include "data_collector.h"
#include <iostream>

using namespace std;

DataCollector::DataCollector(const string& csv_filename,
                             const string& log_filename) {
    csv_file.open(csv_filename);
    log_file.open(log_filename);

    if (!csv_file.is_open()) {
        cerr << "Error opening CSV file\n";
    }

    if (!log_file.is_open()) {
        cerr << "Error opening log file\n";
    }

    // CSV Header
    csv_file << "run_id,avg_latency,throughput,loss_rate\n";
}

DataCollector::~DataCollector() {
    if (csv_file.is_open()) csv_file.close();
    if (log_file.is_open()) log_file.close();
}

void DataCollector::start_run(int id) {
    run_id = id;
    log_file << "\n=== START RUN " << run_id << " ===\n";
}

void DataCollector::record_metrics(const GlobalMetrics& gm) {
    // Write to CSV
    csv_file << run_id << ","
             << gm.avg_latency << ","
             << gm.throughput << ","
             << gm.loss_rate << "\n";

    // Also log
    log_file << "Run " << run_id << " Results:\n";
    log_file << "Latency: " << gm.avg_latency << "\n";
    log_file << "Throughput: " << gm.throughput << "\n";
    log_file << "Loss Rate: " << gm.loss_rate << "\n";
}

void DataCollector::log_event(const string& msg) {
    log_file << msg << endl;
}
