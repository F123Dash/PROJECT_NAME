#include "shortest_path.h"
#include "../performance/profiler.h"
#include <queue>
#include <climits>

using namespace std;

const int INF = INT_MAX;

void dijkstra(Graph &g, int source, vector<int> &dist, vector<int> &parent) {
    ScopedTimer timer("dijkstra");
    
    int n = g.V;
    
    // Pre-allocate vectors (optimization: avoid repeated reallocations)
    dist.reserve(n);
    parent.reserve(n);
    
    dist.assign(n, INF);
    parent.assign(n, -1);

    priority_queue<pair<int,int>, vector<pair<int,int>>, greater<>> pq;

    dist[source] = 0;
    pq.push({0, source});

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();

        if (d > dist[u]) continue;

        // Use const reference to avoid copies
        const auto& neighbors = g.get_neighbors(u);
        for (const auto& [v, w] : neighbors) {
            if (dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                parent[v] = u;
                pq.push({dist[v], v});
            }
        }
    }
}

bool bellmanFord(Graph &g, int source, vector<int> &dist, vector<int> &parent) {
    ScopedTimer timer("bellman_ford");
    
    int n = g.V;
    dist.assign(n, INF);
    parent.assign(n, -1);

    dist[source] = 0;

    for (int i = 0; i < n - 1; i++) {
        for (int u = 0; u < n; u++) {
            const auto& neighbors = g.get_neighbors(u);
            for (const auto& [v, w] : neighbors) {
                if (dist[u] != INF && dist[u] + w < dist[v]) {
                    dist[v] = dist[u] + w;
                    parent[v] = u;
                }
            }
        }
    }

    for (int u = 0; u < n; u++) {
        const auto& neighbors = g.get_neighbors(u);
        for (const auto& [v, w] : neighbors) {
            if (dist[u] != INF && dist[u] + w < dist[v]) {
                return false;
            }
        }
    }

    return true;
}