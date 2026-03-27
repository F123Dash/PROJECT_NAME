#ifndef GRAPH_GENERATOR_H
#define GRAPH_GENERATOR_H

#include "graphs.h"

enum class GraphType {
    RANDOM,
    TREE,
    GRID
};

// Unified generator interface
Graph generate_graph(
    GraphType type,
    int n = 0,          // used for RANDOM & TREE
    double p = 0.0,     // used for RANDOM
    int rows = 0,       // used for GRID
    int cols = 0,       // used for GRID
    int w_min = 1,
    int w_max = 10
);

#endif