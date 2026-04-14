#include "experiment_runner.h"
#include "../engine/pipeline.h"
#include "../graphs/graph_generator.h"
#include "../algorithms/routing_table.h"
#include <iostream>
#include <stdexcept>

ExperimentRunner::ExperimentRunner(const std::string& csv_out,
                                   const std::string& log_out)
    : data_collector(csv_out, log_out), run_counter(0) {}

void ExperimentRunner::add_experiment(const ExperimentConfig& ec) {
    experiments.push_back(ec);
}

void ExperimentRunner::run_all() {
    for (const auto& ec : experiments) {
        try {
            run_single(ec);
        } catch (const std::exception& e) {
            data_collector.log_error(
                "[ExperimentRunner] Run " + std::to_string(run_counter) +
                " failed: " + e.what()
            );
        }
    }
}

void ExperimentRunner::run_single(const ExperimentConfig& ec) {
    run_counter++;
    data_collector.start_run(run_counter);

    data_collector.log_step("INIT",
        "nodes=" + std::to_string(ec.num_nodes) +
        " traffic_load=" + std::to_string(ec.traffic_load) +
        " routing=" + ec.routing_algorithm
    );

    // 1. Build simulator and load base config
    Simulator sim;
    sim.load_config(ec.base_config);

    // 2. Override topology
    Graph g = generate_graph(
        ec.graph_type,
        ec.num_nodes,
        ec.edge_probability,
        ec.grid_rows,
        ec.grid_cols
    );
    sim.graph = &g;

    // 3. Override routing algorithm in config
    sim.config.data["routing_algorithm"] = ec.routing_algorithm;

    // 4. Override traffic load
    sim.config.data["traffic_load"] = std::to_string(ec.traffic_load);

    // 5. Init and run via pipeline
    sim.init_system();
    sim.init_traffic();

    Pipeline pipeline(&sim, &data_collector);
    pipeline.execute();

    data_collector.log_step("DONE", "Run " + std::to_string(run_counter) + " complete");
}
