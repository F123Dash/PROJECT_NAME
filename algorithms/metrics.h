#ifndef METRICS_H
#define METRICS_H

#include <vector>
#include "../graphs/graphs.h"

// Stores basic path information
struct PathMetrics {
    int path_length;   // number of edges
    int path_cost;     // total weight
};

// Compute length + cost of a given path
PathMetrics compute_path_metrics(const std::vector<int>& path, Graph& g);

// Compute path in MST (tree traversal)
std::vector<int> mst_path(Graph& mst, int src, int dest);

// Compute stretch = MST cost / shortest path cost
double path_stretch(int shortest_cost, int mst_cost);

#endif
