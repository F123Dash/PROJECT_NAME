#include "metrics.h"
#include <queue>
#include <algorithm>

using namespace std;

// path length + cost
PathMetrics compute_path_metrics(const vector<int>& path, Graph& g) {
    int cost = 0;

    for (int i = 0; i < (int)path.size() - 1; i++) {
        int u = path[i];
        int v = path[i + 1];

        for (auto &nbr : g.adj[u]) {
            if (nbr.first == v) {
                cost += nbr.second;
                break;
            }
        }
    }

    return { (int)path.size() - 1, cost };
}

//path inside MST (tree)
vector<int> mst_path(Graph& mst, int src, int dest) {
    int n = mst.V;

    vector<int> parent(n, -1);
    vector<bool> vis(n, false);

    queue<int> q;
    q.push(src);
    vis[src] = true;

    while (!q.empty()) {
        int u = q.front();
        q.pop();

        for (auto &[v, w] : mst.adj[u]) {
            if (!vis[v]) {
                vis[v] = true;
                parent[v] = u;
                q.push(v);
            }
        }
    }

    // reconstruct path
    vector<int> path;
    for (int v = dest; v != -1; v = parent[v]) {
        path.push_back(v);
    }

    reverse(path.begin(), path.end());
    return path;
}

// Path stretch
double path_stretch(int shortest_cost, int mst_cost) {
    if (shortest_cost == 0) return 0;
    return (double)mst_cost / shortest_cost;
}
