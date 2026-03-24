#ifndef GRAPH_H
#define GRAPH_H

#include <stdexcept>
#include <utility>
#include <vector>

class Graph {
public:
  // number of vertices
  int V;
  // adjacency list
  std::vector<std::vector<std::pair<int, int>>> adj;

  // Constructor
  Graph(int n);

  // Operations
  void add_edge(int u, int v, int w);
  void remove_edge(int u, int v);
  std::vector<std::pair<int, int>> get_neighbors(int u) const;

  // utility
  void validate_node(int u) const;
};

#endif // !GRAPH_H
