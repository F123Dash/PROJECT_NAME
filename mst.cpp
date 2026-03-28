#include <bits/stdc++.h>
#include "../include/union_find.h"
#include <fstream>
using namespace std;

struct Edge {
    int u, v, w;
};

//Kruskal's Algorithm
void kruskal(int n, vector<Edge>& edges) {
    sort(edges.begin(), edges.end(), [](Edge a, Edge b) {
        return a.w < b.w;
    });

    UnionFind uf(n);
    int total_cost = 0;

    cout << "Kruskal MST:\n";

    for (auto &e : edges) {
        if (uf.find(e.u) != uf.find(e.v)) {
            uf.unite(e.u, e.v);
            cout << e.u << " " << e.v << " " << e.w << "\n";
            total_cost += e.w;
        }
    }

    cout << "Total cost: " << total_cost << "\n\n";
}

// Prim's Algorithm
void prim(int n, vector<vector<pair<int,int>>>& adj) {
    vector<bool> vis(n, false);

    priority_queue<
        tuple<int,int,int>,
        vector<tuple<int,int,int>>,
        greater<>
    > pq;

    pq.push({0, 0, -1}); // {weight, node, parent}
    int total_cost = 0;

    cout << "Prim MST:\n";

    while (!pq.empty()) {
        auto [w, u, parent] = pq.top();
        pq.pop();

        if (vis[u]) continue;

        vis[u] = true;
        total_cost += w;

        if (parent != -1)
            cout << parent << " " << u << " " << w << "\n";

        for (auto &[v, wt] : adj[u]) {
            if (!vis[v]) {
                pq.push({wt, v, u});
            }            
        }
    }

    cout << "Total cost: " << total_cost << "\n";
}


//main function
int main() {
    ifstream file("input.txt");

    int n, m;
    file >> n >> m;

    vector<Edge> edges;

    for (int i = 0; i < m; i++) {
        int u, v, w;
        file >> u >> v >> w;
        edges.push_back({u, v, w});
    }

    vector<vector<pair<int,int>>> adj(n);
    for (auto &e : edges) {
        adj[e.u].push_back({e.v, e.w});
        adj[e.v].push_back({e.u, e.w});
    }

    kruskal(n, edges);
    prim(n, adj);

    return 0;
}
