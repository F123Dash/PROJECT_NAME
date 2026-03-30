#ifndef TOPOLOGY_LOADER_H
#define TOPOLOGY_LOADER_H

#include <iostream>
#include <fstream>
#include <vector>
#include <tuple>
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

Graph load_graph(const string &filename);

#endif
