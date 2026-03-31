#ifndef GRAPH_ANALYSIS_H
#define GRAPH_ANALYSIS_H

#include <vector>
#include "graphs.h"
using namespace std;

// Analysis functions
int degree(const Graph &g, int u);
double average_degree(const Graph &g);
vector<int> bfs(const Graph &g, int src);
int diameter(const Graph &g);
double average_path_length(const Graph &g);

Graph load_graph(const string &filename);

#endif
