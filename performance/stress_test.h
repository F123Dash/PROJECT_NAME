#ifndef STRESS_TEST_H
#define STRESS_TEST_H

#include "../graphs/graphs.h"
#include "../graphs/graph_generator.h"
#include "../engine/integration.h"
#include "profiler.h"
#include <iostream>
#include <vector>

// Stress Test Generator
// Tests simulator performance with varying network sizes

class StressTest {
public:
    struct TestResult {
        int nodes;
        int edges;
        double init_time;
        double routing_time;
        double total_time;
        bool success;
        std::string error_msg;
    };

    // Run single stress test
    static TestResult runTest(int num_nodes, double edge_probability);

    // Run progressive stress tests (100, 500, 1000, 5000, 10000 nodes)
    static void runProgressiveTests();

    // Run stress test suite with custom node counts
    static void runCustomTests(const std::vector<int>& node_counts);

private:
    StressTest() = default;
};

#endif // STRESS_TEST_H