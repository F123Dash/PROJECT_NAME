#include <iostream>
#include <fstream>
#include <vector>
#include <tuple>
#include <queue>
#include "graph_analysis.h"
using namespace std;

int degree(const Graph &g, int u) {
    return g.adj[u].size();
}

double average_degree(const Graph &g) {
    int total = 0;
    for (int i = 0; i < g.V; i++) {
        total += g.adj[i].size();
    }
    return (double)total / g.V;
}

vector<int> bfs(const Graph &g, int src) {
    vector<int> dist(g.V, -1);
    queue<int> q;

    dist[src] = 0;
    q.push(src);

    while (!q.empty()) {
        int u = q.front(); q.pop();
        for (auto &p : g.adj[u]) {
            int v = p.first;
            if (dist[v] == -1) {
                dist[v] = dist[u] + 1;
                q.push(v);
            }
        }
    }
    return dist;
}

int diameter(const Graph &g) {
    int dia = 0;
    for (int i = 0; i < g.V; i++) {
        vector<int> dist = bfs(g, i);
        for (int d : dist) {
            if (d > dia) dia = d;
        }
    }
    return dia;
}

double average_path_length(const Graph &g) {
    double total = 0;
    int count = 0;

    for (int i = 0; i < g.V; i++) {
        vector<int> dist = bfs(g, i);
        for (int j = 0; j < g.V; j++) {
            if (i != j && dist[j] != -1) {
                total += dist[j];
                count++;
            }
        }
    }

    return (count == 0) ? 0 : total / count;
}

Graph load_graph(const string &filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("Error: Cannot open file");
    }

    vector<tuple<int,int,int>> edges;
    int u, v, w;
    int max_node = 0;

    while (file >> u >> v >> w) {
        edges.push_back({u, v, w});
        if (u > max_node) max_node = u;
        if (v > max_node) max_node = v;
    }

    Graph g(max_node + 1);

    for (auto &e : edges) {
        tie(u, v, w) = e;
        g.add_edge(u, v, w);
    }

    return g;
}