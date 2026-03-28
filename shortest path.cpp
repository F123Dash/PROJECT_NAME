#include "shortest_path.h"
#include <queue>
#include <climits>

using namespace std;

const int INF = INT_MAX;

void dijkstra(Graph &g, int source, vector<int> &dist, vector<int> &parent) {
    int n = g.V;
    dist.assign(n, INF);
    parent.assign(n, -1);

    priority_queue<pair<int,int>, vector<pair<int,int>>, greater<>> pq;

    dist[source] = 0;
    pq.push({0, source});

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();

        if (d > dist[u]) continue;

        for (auto [v, w] : g.get_neighbors(u)) {
            if (dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                parent[v] = u;
                pq.push({dist[v], v});
            }
        }
    }
}

bool bellmanFord(Graph &g, int source, vector<int> &dist, vector<int> &parent) {
    int n = g.V;
    dist.assign(n, INF);
    parent.assign(n, -1);

    dist[source] = 0;

    for (int i = 0; i < n - 1; i++) {
        for (int u = 0; u < n; u++) {
            for (auto [v, w] : g.get_neighbors(u)) {
                if (dist[u] != INF && dist[u] + w < dist[v]) {
                    dist[v] = dist[u] + w;
                    parent[v] = u;
                }
            }
        }
    }

    for (int u = 0; u < n; u++) {
        for (auto [v, w] : g.get_neighbors(u)) {
            if (dist[u] != INF && dist[u] + w < dist[v]) {
                return false;
            }
        }
    }

    return true;
}
 
