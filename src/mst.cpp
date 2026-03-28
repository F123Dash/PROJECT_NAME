#include "mst.h"
#include "../include/union_find.h"
#include <queue>
#include <tuple>
#include <algorithm>

using namespace std;

// Kruskal
vector<Edge> kruskal(int n, vector<Edge>& edges) {
    sort(edges.begin(), edges.end(), [](Edge a, Edge b) {
        return a.w < b.w;
    });

    UnionFind uf(n);
    vector<Edge> mst;

    for (auto &e : edges) {
        if (uf.find(e.u) != uf.find(e.v)) {
            uf.unite(e.u, e.v);
            mst.push_back(e);
        }
    }

    return mst;
}

// Prim using Graph class
vector<Edge> prim(Graph& g) {
    int n = g.V;
    vector<bool> vis(n, false);

    priority_queue<
        tuple<int,int,int>,
        vector<tuple<int,int,int>>,
        greater<>
    > pq;

    vector<Edge> mst;

    pq.push({0, 0, -1});

    while (!pq.empty()) {
        auto [w, u, parent] = pq.top();
        pq.pop();

        if (vis[u]) continue;

        vis[u] = true;

        if (parent != -1)
            mst.push_back({parent, u, w});

        for (auto &[v, wt] : g.adj[u]) {
            if (!vis[v]) {
                pq.push({wt, v, u});
            }
        }
    }

    return mst;
}
