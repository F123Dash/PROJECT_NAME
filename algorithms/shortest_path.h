#ifndef SHORTEST_PATH_H
#define SHORTEST_PATH_H

#include <vector>
#include "../graphs/graphs.h"

void dijkstra(Graph &g, int source, std::vector<int> &dist, std::vector<int> &parent);

bool bellmanFord(Graph &g, int source, std::vector<int> &dist, std::vector<int> &parent);

#endif
