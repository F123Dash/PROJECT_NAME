#ifndef MST_H
#define MST_H

#include <vector>
#include "../graphs/graphs.h"

struct Edge {
    int u, v, w;
};

std::vector<Edge> kruskal(int n, std::vector<Edge>& edges);
std::vector<Edge> prim(Graph& g);

#endif
