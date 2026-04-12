#ifndef GRAPH_H
#define GRAPH_H

#include <stdexcept>
#include <utility>
#include <vector>
#include <map>
#include "link.h"

class Graph {
public:
  // number of vertices
  int V;
  // adjacency list
  std::vector<std::vector<std::pair<int, int>>> adj;
  
  // Link properties: (from, to) -> Link
  std::map<std::pair<int, int>, Link> links;

  // Constructor
  Graph(int n);

  // Operations
  void add_edge(int u, int v, int w);
  void remove_edge(int u, int v);
  std::vector<std::pair<int, int>> get_neighbors(int u) const;
  
  // Link property operations
  void setLinkProperties(int u, int v, double bandwidth, double latency);
  Link* getLinkProperties(int u, int v);
  void setDefaultLinkProperties(double bandwidth, double latency);

  // utility
  void validate_node(int u) const;
};

#endif // !GRAPH_H
