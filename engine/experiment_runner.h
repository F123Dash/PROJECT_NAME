#ifndef EXPERIMENT_RUNNER_H
#define EXPERIMENT_RUNNER_H

#include "../engine/simulator.h"
#include "../engine/data_collector.h"
#include "../graphs/graph_generator.h"
#include <vector>
#include <string>

struct ExperimentConfig {
    // Topology
    GraphType graph_type;
    int num_nodes;
    double edge_probability;  // for RANDOM
    int grid_rows, grid_cols; // for GRID

    // Traffic
    double traffic_load;      // packets per second scale

    // Routing
    std::string routing_algorithm; // "shortest_path" | "mst"

    // Base config file to load defaults from
    std::string base_config;
};

class ExperimentRunner {
public:
    ExperimentRunner(const std::string& csv_out, const std::string& log_out);

    void add_experiment(const ExperimentConfig& ec);
    void run_all();

private:
    std::vector<ExperimentConfig> experiments;
    DataCollector data_collector;
    int run_counter;

    void run_single(const ExperimentConfig& ec);
};

#endif
