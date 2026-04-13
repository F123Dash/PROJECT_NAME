#ifndef DATA_COLLECTOR_H
#define DATA_COLLECTOR_H

#include <string>
#include <fstream>
#include "../network/metrics_types.h"

class DataCollector {
private:
    std::ofstream csv_file;
    std::ofstream log_file;
    int run_id;

public:
    DataCollector(const std::string& csv_filename,
                  const std::string& log_filename);

    ~DataCollector();

    void start_run(int id);

    void record_metrics(const GlobalMetrics& gm);

    void log_event(const std::string& msg);
    
    void log_step(const std::string& step, const std::string& description);
    
    void log_error(const std::string& error_msg);
};

#endif
