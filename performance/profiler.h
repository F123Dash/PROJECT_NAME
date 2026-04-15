#ifndef PROFILER_H
#define PROFILER_H

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>
#include <iomanip>
#include <iostream>

// Performance Profiler - Singleton Pattern
// Tracks execution time of critical sections throughout the simulator

class Profiler {
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;
    using Duration = std::chrono::duration<double>;

    struct TimingEntry {
        std::string name;
        int count;
        double total_time;
        double min_time;
        double max_time;
        double avg_time;
    };

    // Singleton access
    static Profiler* getInstance();
    static void destroyInstance();

    // Start/stop timing for named sections
    void start(const std::string& name);
    void stop(const std::string& name);

    // Reset all timings
    void reset();

    // Generate report
    void report(std::ostream& out = std::cout);

    // Query methods
    double getTotalTime(const std::string& name) const;
    int getCount(const std::string& name) const;
    double getAvgTime(const std::string& name) const;

    // Export to CSV
    void exportCSV(const std::string& filename);

    // Enable/disable profiling globally
    static void enable() { enabled = true; }
    static void disable() { enabled = false; }
    static bool isEnabled() { return enabled; }

private:
    Profiler() = default;
    static Profiler* instance;
    static bool enabled;

    struct Timing {
        TimePoint start_point;
        int count;
        double total_time;
        double min_time;
        double max_time;
    };

    std::unordered_map<std::string, Timing> timings;
};

// RAII Timer - Automatically start/stop profiling scope

class ScopedTimer {
public:
    explicit ScopedTimer(const std::string& name) : name(name) {
        if (Profiler::isEnabled()) {
            Profiler::getInstance()->start(name);
        }
    }

    ~ScopedTimer() {
        if (Profiler::isEnabled()) {
            Profiler::getInstance()->stop(name);
        }
    }

private:
    std::string name;
};

#endif // PROFILER_H