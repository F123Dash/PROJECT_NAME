#include "stress_test.h"
#include <chrono>
#include <iomanip>

using namespace std;
using Clock = chrono::high_resolution_clock;

StressTest::TestResult StressTest::runTest(int num_nodes, double edge_probability) {
    TestResult result{num_nodes, 0, 0.0, 0.0, 0.0, false, ""};
    
    try {
        cout << "\n[STRESS TEST] Starting test: " << num_nodes << " nodes, "
             << edge_probability << " edge probability\n";
        cout << string(60, '-') << "\n";

        Profiler* profiler = Profiler::getInstance();
        profiler->reset();
        Profiler::enable();

        // Phase 1: Generate graph
        auto start = Clock::now();
        cout << "  [1/3] Generating graph... ";
        cout.flush();
        
        profiler->start("stress_graph_generation");
        Graph graph = generate_graph(GraphType::RANDOM, num_nodes, edge_probability, 0, 0, 1, 10);
        profiler->stop("stress_graph_generation");
        
        result.edges = 0;
        for (int i = 0; i < num_nodes; i++) {
            result.edges += graph.adj[i].size();
        }
        result.edges /= 2;  // Undirected graph
        
        auto gen_time = chrono::duration<double>(Clock::now() - start).count();
        cout << "done (" << fixed << setprecision(3) << gen_time << "s, "
             << result.edges << " edges)\n";

        // Phase 2: Initialize system
        cout << "  [2/3] Initializing system... ";
        cout.flush();
        
        start = Clock::now();
        profiler->start("stress_system_init");
        
        Integration system(&graph);
        system.init_nodes();
        system.attach_layers();
        
        profiler->stop("stress_system_init");
        auto init_time = chrono::duration<double>(Clock::now() - start).count();
        result.init_time = init_time;
        cout << "done (" << fixed << setprecision(3) << init_time << "s)\n";

        // Phase 3: Build routing tables
        cout << "  [3/3] Building routing tables... ";
        cout.flush();
        
        start = Clock::now();
        profiler->start("stress_routing_tables");
        system.build_routing_tables();
        profiler->stop("stress_routing_tables");
        
        auto routing_time = chrono::duration<double>(Clock::now() - start).count();
        result.routing_time = routing_time;
        cout << "done (" << fixed << setprecision(3) << routing_time << "s)\n";

        result.total_time = gen_time + init_time + routing_time;
        result.success = true;

        cout << "\n  Total time: " << fixed << setprecision(3) << result.total_time << "s\n";
        cout << string(60, '-') << "\n";

        Profiler::disable();
        
    } catch (const exception& e) {
        result.success = false;
        result.error_msg = string(e.what());
        cerr << "\n[ERROR] " << result.error_msg << "\n";
    }

    return result;
}

void StressTest::runProgressiveTests() {
    vector<int> node_counts = {100, 500, 1000, 5000, 10000};
    double edge_prob = 0.01;
    
    cout << "\n" << string(70, '=') << "\n";
    cout << "PROGRESSIVE STRESS TEST SUITE\n";
    cout << string(70, '=') << "\n";

    vector<TestResult> results;
    
    for (int nodes : node_counts) {
        TestResult result = runTest(nodes, edge_prob);
        results.push_back(result);
        
        // Early exit if test fails or takes too long
        if (!result.success || result.total_time > 120.0) {
            cerr << "[STRESS TEST] Stopping: ";
            if (!result.success) {
                cerr << "Test failed\n";
            } else {
                cerr << "Test took too long (> 120s)\n";
            }
            break;
        }
    }

    // Summary
    cout << "\n" << string(70, '=') << "\n";
    cout << "SUMMARY\n";
    cout << string(70, '=') << "\n";
    cout << left << setw(12) << "Nodes"
         << right << setw(12) << "Edges"
         << setw(15) << "Init (s)"
         << setw(15) << "Routing (s)"
         << setw(15) << "Total (s)" << "\n";
    cout << string(70, '-') << "\n";

    for (const auto& result : results) {
        cout << left << setw(12) << result.nodes
             << right << setw(12) << result.edges
             << setw(15) << fixed << setprecision(4) << result.init_time
             << setw(15) << fixed << setprecision(4) << result.routing_time
             << setw(15) << fixed << setprecision(4) << result.total_time << "\n";
    }

    cout << string(70, '=') << "\n\n";
}

void StressTest::runCustomTests(const vector<int>& node_counts) {
    cout << "\n" << string(70, '=') << "\n";
    cout << "CUSTOM STRESS TEST SUITE\n";
    cout << string(70, '=') << "\n";

    vector<TestResult> results;
    double edge_prob = 0.01;

    for (int nodes : node_counts) {
        TestResult result = runTest(nodes, edge_prob);
        results.push_back(result);
        
        if (!result.success || result.total_time > 120.0) {
            break;
        }
    }

    // Summary
    cout << "\n" << string(70, '=') << "\n";
    cout << "SUMMARY\n";
    cout << string(70, '=') << "\n";
    cout << left << setw(12) << "Nodes"
         << right << setw(12) << "Edges"
         << setw(15) << "Init (s)"
         << setw(15) << "Routing (s)"
         << setw(15) << "Total (s)" << "\n";
    cout << string(70, '-') << "\n";

    for (const auto& result : results) {
        cout << left << setw(12) << result.nodes
             << right << setw(12) << result.edges
             << setw(15) << fixed << setprecision(4) << result.init_time
             << setw(15) << fixed << setprecision(4) << result.routing_time
             << setw(15) << fixed << setprecision(4) << result.total_time << "\n";
    }

    cout << string(70, '=') << "\n\n";
}