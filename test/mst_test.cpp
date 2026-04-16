#include <iostream>
#include <vector>
#include "../graphs/graphs.h"
#include "../algorithms/mst.h"

static int total_cost(const std::vector<Edge>& edges) {
    int cost = 0;
    for (const auto& e : edges) cost += e.w;
    return cost;
}

int main() {
    Graph g(4);
    g.add_edge(0, 1, 1);
    g.add_edge(1, 2, 2);
    g.add_edge(2, 3, 3);
    g.add_edge(0, 3, 50);

    std::vector<Edge> prim_edges = prim(g);
    int prim_cost = total_cost(prim_edges);

    std::vector<Edge> all_edges = {
        {0, 1, 1}, {1, 2, 2}, {2, 3, 3}, {0, 3, 50}
    };
    std::vector<Edge> kruskal_edges = kruskal(g.V, all_edges);
    int kruskal_cost = total_cost(kruskal_edges);

    std::cout << "prim_cost=" << prim_cost << "\n";
    std::cout << "kruskal_cost=" << kruskal_cost << "\n";
    return 0;
}
