#include "comparison.h"

#include "../algorithms/mst.h"
#include "../algorithms/shortest_path.h"
#include <algorithm>
#include <climits>
#include <iostream>
#include <limits>
using namespace std;

// 🔧 helper: reconstruct path from parent array
vector<int> build_path(int src, int dest, vector<int> &parent) {
    vector<int> path;
    int cur = dest;

    while (cur != -1) {
        path.push_back(cur);
        if (cur == src) break;
        cur = parent[cur];
    }

    reverse(path.begin(), path.end());
    return path;
}

ComparisonResult Comparison::compare(Graph &g, int source, int destination) {
    ComparisonResult result;

    int n = g.V;

    // SHORTEST PATH (DIJKSTRA)
    vector<int> dist(n, INT_MAX);
    vector<int> parent(n, -1);

    dijkstra(g, source, dist, parent);

    vector<int> sp_path = build_path(source, destination, parent);

    result.sp_cost = dist[destination];
    result.sp_latency = sp_path.size();

    // MST (PRIM)
    vector<Edge> mst_edges = prim(g);

    // Build MST graph
    Graph mst_graph(n);

    for (auto &e : mst_edges) {
        mst_graph.add_edge(e.u, e.v, e.w);
    }

    // Run dijkstra again on MST
    vector<int> mst_dist(n, INT_MAX);
    vector<int> mst_parent(n, -1);

    dijkstra(mst_graph, source, mst_dist, mst_parent);

    vector<int> mst_path = build_path(source, destination, mst_parent);

    result.mst_cost = mst_dist[destination];
    result.mst_latency = mst_path.size();

    // PATH STRETCH
    if (result.sp_cost > 0)
        result.path_stretch = (double)result.mst_cost / result.sp_cost;
    else
        result.path_stretch = numeric_limits<double>::infinity();

    // OUTPUT
    cout << "\n--- Algorithm Comparison ---\n";

    cout << "Shortest Path Cost: " << result.sp_cost << endl;
    cout << "MST Path Cost: " << result.mst_cost << endl;

    cout << "Shortest Path Latency (hops): " << result.sp_latency << endl;
    cout << "MST Latency (hops): " << result.mst_latency << endl;

    cout << "Path Stretch: " << result.path_stretch << endl;

    return result;
}
