#include <iostream>
#include <fstream>
#include <vector>
#include <tuple>
#include "topology_loader.h"
using namespace std;

class Graph {
public:
    int V;
    vector<vector<pair<int,double>>> adj;

    Graph(int n) {
        V = n;
        adj.resize(V);
    }

    void add_edge(int u, int v, double w) {
        adj[u].push_back({v, w});
        adj[v].push_back({u, w});
    }

    vector<pair<int,double>> get_neighbors(int u) {
        return adj[u];
    }

    void print() {
        for (int i = V - 1; i >= 0; i--) {
            cout << "Node " << i << ": ";
            for (auto &p : adj[i]) {
                cout << "(" << p.first << ", " << p.second << ") ";
            }
            cout << endl;
        }
    }
};

Graph load_graph(const string &filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("Error: Cannot open file");
    }

    vector<tuple<int,int,double>> edges;
    int u, v;
    double w;
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