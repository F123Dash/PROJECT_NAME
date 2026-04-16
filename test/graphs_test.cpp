#include <iostream>
#include "../graphs/graphs.h"

int main() {
    Graph g(4);

    try {
        g.add_edge(0, 99, 1);
        std::cout << "boundary_not_checked\n";
    } catch (const std::out_of_range&) {
        std::cout << "boundary_checked\n";
    }

    g.add_edge(1, 2, 7);
    g.remove_edge(1, 2);

    bool symmetric = true;
    for (auto [v, w] : g.adj[1]) {
        if (v == 2) symmetric = false;
    }
    for (auto [v, w] : g.adj[2]) {
        if (v == 1) symmetric = false;
    }

    std::cout << (symmetric ? "remove_edge_symmetric\n" : "remove_edge_not_symmetric\n");
    return 0;
}
