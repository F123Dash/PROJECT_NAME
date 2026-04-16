#include <climits>
#include <iostream>
#include <vector>
#include "../graphs/graphs.h"
#include "../algorithms/shortest_path.h"

int main() {
    Graph g1(4);
    g1.add_edge(0, 1, 5);
    g1.add_edge(1, 2, 3);

    std::vector<int> dist1, parent1;
    dijkstra(g1, 0, dist1, parent1);
    std::cout << "dijkstra_dist_3=" << (dist1[3] == INT_MAX ? -1 : dist1[3]) << "\n";

    Graph g2(3);
    g2.add_edge(0, 1, 1);
    g2.add_edge(1, 2, -1);
    g2.add_edge(2, 1, -1);

    std::vector<int> dist2, parent2;
    bool ok = bellmanFord(g2, 0, dist2, parent2);
    std::cout << (ok ? "bellman_no_cycle_reported\n" : "bellman_cycle_reported\n");
    return 0;
}
