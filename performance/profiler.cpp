#include "profiler.h"
#include <fstream>
#include <iomanip>
#include <algorithm>

using namespace std;

Profiler* Profiler::instance = nullptr;
bool Profiler::enabled = false;

// Singleton Implementation

Profiler* Profiler::getInstance() {
    if (!instance) {
        instance = new Profiler();
    }
    return instance;
}

void Profiler::destroyInstance() {
    if (instance) {
        delete instance;
        instance = nullptr;
    }
}

// Timing Methods

void Profiler::start(const std::string& name) {
    if (!enabled) return;
    timings[name].start_point = Clock::now();
}

void Profiler::stop(const std::string& name) {
    if (!enabled) return;
    
    auto it = timings.find(name);
    if (it == timings.end()) {
        // First entry for this name
        timings[name] = {Clock::now(), 0, 0.0, 1e9, 0.0};
    }

    auto end = Clock::now();
    Duration elapsed = end - timings[name].start_point;
    double elapsed_sec = elapsed.count();

    // Update statistics
    timings[name].count++;
    timings[name].total_time += elapsed_sec;
    timings[name].min_time = std::min(timings[name].min_time, elapsed_sec);
    timings[name].max_time = std::max(timings[name].max_time, elapsed_sec);
}

void Profiler::reset() {
    timings.clear();
}

// Reporting

void Profiler::report(std::ostream& out) {
    if (timings.empty()) {
        out << "\n[PROFILER] No timing data collected\n";
        return;
    }

    // Collect sorted entries by total time (descending)
    vector<TimingEntry> entries;
    
    for (auto& p : timings) {
        const auto& timing = p.second;
        double avg = timing.count > 0 ? timing.total_time / timing.count : 0.0;
        
        entries.push_back({
            p.first,
            timing.count,
            timing.total_time,
            timing.min_time,
            timing.max_time,
            avg
        });
    }

    // Sort by total time (descending)
    sort(entries.begin(), entries.end(), 
         [](const TimingEntry& a, const TimingEntry& b) {
             return a.total_time > b.total_time;
         });

    // Print report
    out << "\n" << string(85, '=') << "\n";
    out << "PROFILING REPORT\n";
    out << string(85, '=') << "\n";
    
    out << left << setw(25) << "Section" 
        << right << setw(10) << "Count"
        << setw(15) << "Total (s)"
        << setw(12) << "Avg (ms)"
        << setw(12) << "Min (ms)"
        << setw(12) << "Max (ms)" << "\n";
    
    out << string(85, '-') << "\n";

    for (const auto& entry : entries) {
        out << left << setw(25) << entry.name
            << right << setw(10) << entry.count
            << setw(15) << fixed << setprecision(4) << entry.total_time
            << setw(12) << fixed << setprecision(3) << (entry.avg_time * 1000.0)
            << setw(12) << fixed << setprecision(3) << (entry.min_time * 1000.0)
            << setw(12) << fixed << setprecision(3) << (entry.max_time * 1000.0) << "\n";
    }
    
    out << string(85, '=') << "\n";

    // Print summary
    double total = 0.0;
    for (const auto& entry : entries) {
        total += entry.total_time;
    }
    
    out << "\nTotal combined time: " << fixed << setprecision(4) << total << " seconds\n\n";
}

// Query Methods

double Profiler::getTotalTime(const std::string& name) const {
    auto it = timings.find(name);
    if (it != timings.end()) {
        return it->second.total_time;
    }
    return 0.0;
}

int Profiler::getCount(const std::string& name) const {
    auto it = timings.find(name);
    if (it != timings.end()) {
        return it->second.count;
    }
    return 0;
}

double Profiler::getAvgTime(const std::string& name) const {
    auto it = timings.find(name);
    if (it != timings.end()) {
        const auto& timing = it->second;
        return timing.count > 0 ? timing.total_time / timing.count : 0.0;
    }
    return 0.0;
}

// CSV Export

void Profiler::exportCSV(const std::string& filename) {
    ofstream out(filename);
    if (!out.is_open()) {
        cerr << "[PROFILER] Error opening CSV file: " << filename << "\n";
        return;
    }

    // Header
    out << "section,count,total_time_s,avg_time_ms,min_time_ms,max_time_ms\n";

    // Data
    vector<TimingEntry> entries;
    for (auto& p : timings) {
        const auto& timing = p.second;
        double avg = timing.count > 0 ? timing.total_time / timing.count : 0.0;
        
        entries.push_back({
            p.first,
            timing.count,
            timing.total_time,
            timing.min_time,
            timing.max_time,
            avg
        });
    }

    sort(entries.begin(), entries.end(),
         [](const TimingEntry& a, const TimingEntry& b) {
             return a.total_time > b.total_time;
         });

    for (const auto& entry : entries) {
        out << entry.name << ","
            << entry.count << ","
            << fixed << setprecision(6) << entry.total_time << ","
            << fixed << setprecision(3) << (entry.avg_time * 1000.0) << ","
            << fixed << setprecision(3) << (entry.min_time * 1000.0) << ","
            << fixed << setprecision(3) << (entry.max_time * 1000.0) << "\n";
    }

    out.close();
    cout << "[PROFILER] CSV report exported to: " << filename << "\n";
}