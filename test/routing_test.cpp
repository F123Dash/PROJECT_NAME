#include <climits>
#include <iostream>
#include <vector>
#include "../graphs/graphs.h"
#include "../algorithms/shortest_path.h"
#include "../algorithms/routing_table.h"

int main() {
    Graph g(5);
    g.add_edge(0, 1, 1);
    g.add_edge(1, 2, 1);
    g.add_edge(2, 3, 1);

    std::vector<int> dist, parent;
    dijkstra(g, 0, dist, parent);
    std::vector<int> routing = buildRoutingTable(parent, dist, 0, g.V);

    int destination = 3;
    int current = 1;
    int guard = 0;

    // Intentional old misuse: keep using source-0 routing row at intermediate nodes.
    while (current != destination && guard < 8) {
        int next = routing[destination];
        std::cout << current << "->" << next << " ";
        current = next;
        ++guard;
    }
    std::cout << "\n";
    if (guard >= 8) std::cout << "loop_detected\n";

    int unreachable = 4;
    if (dist[unreachable] == INT_MAX) {
        std::cout << "unreachable_next_hop=" << routing[unreachable] << "\n";
    }

    return 0;
}
