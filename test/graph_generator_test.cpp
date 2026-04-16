#include <iostream>
#include <queue>
#include <vector>
#include "../graphs/graph_generator.h"

static bool connected(const Graph& g, int src) {
    std::vector<int> vis(g.V, 0);
    std::queue<int> q;
    q.push(src);
    vis[src] = 1;

    while (!q.empty()) {
        int u = q.front();
        q.pop();
        for (auto [v, w] : g.adj[u]) {
            if (!vis[v]) {
                vis[v] = 1;
                q.push(v);
            }
        }
    }

    for (int f : vis) {
        if (!f) return false;
    }
    return true;
}

int main() {
    Graph random_g = generate_graph(GraphType::RANDOM, 12, 0.05, 0, 0, 1, 5);
    std::cout << (connected(random_g, 0) ? "random_connected\n" : "random_disconnected\n");

    Graph grid_g = generate_graph(GraphType::GRID, 0, 0.0, 2, 5, 1, 3);
    std::cout << "grid_nodes=" << grid_g.V << " first_deg=" << grid_g.adj[0].size() << "\n";
    return 0;
}
