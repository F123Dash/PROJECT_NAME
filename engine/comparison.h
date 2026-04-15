#ifndef COMPARISON_H
#define COMPARISON_H

#include "../graphs/graphs.h"

struct ComparisonResult {
    double mst_cost;
    double sp_cost;

    double mst_latency;
    double sp_latency;

    double path_stretch;
};

class Comparison {
public:
    ComparisonResult compare(Graph &g, int source, int destination);
};

#endif
