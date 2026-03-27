#include <iostream>
#include <vector>
#include <queue>
#include <limits>
using namespace std;

class Graph {
public:
    int V;
    vector<vector<pair<int,int>>> adj;

    Graph(int n) {
        V = n;
        adj.resize(V);
    }

    void add_edge(int u, int v, int w) {
        adj[u].push_back({v, w});
        adj[v].push_back({u, w}); // undirected
    }

    void remove_edge(int u, int v) {
        for (auto it = adj[u].begin(); it != adj[u].end(); ++it) {
            if (it->first == v) {
                adj[u].erase(it);
                break;
            }
        }
        for (auto it = adj[v].begin(); it != adj[v].end(); ++it) {
            if (it->first == u) {
                adj[v].erase(it);
                break;
            }
        }
    }

    vector<pair<int,int>> get_neighbors(int u) {
        return adj[u];
    }

    // ----------- ANALYSIS FUNCTIONS -----------

    int degree(int u) {
        return adj[u].size();
    }

    double average_degree() {
        int total = 0;
        for (int i = 0; i < V; i++) {
            total += adj[i].size();
        }
        return (double)total / V;
    }

    // BFS to compute shortest path distances (unweighted)
    vector<int> bfs(int src) {
        vector<int> dist(V, -1);
        queue<int> q;

        dist[src] = 0;
        q.push(src);

        while (!q.empty()) {
            int u = q.front(); q.pop();
            for (auto &p : adj[u]) {
                int v = p.first;
                if (dist[v] == -1) {
                    dist[v] = dist[u] + 1;
                    q.push(v);
                }
            }
        }
        return dist;
    }

    // Diameter (max shortest path)
    int diameter() {
        int dia = 0;
        for (int i = 0; i < V; i++) {
            vector<int> dist = bfs(i);
            for (int d : dist) {
                if (d > dia) dia = d;
            }
        }
        return dia;
    }

    // Average shortest path length
    double average_path_length() {
        double total = 0;
        int count = 0;

        for (int i = 0; i < V; i++) {
            vector<int> dist = bfs(i);
            for (int j = 0; j < V; j++) {
                if (i != j && dist[j] != -1) {
                    total += dist[j];
                    count++;
                }
            }
        }

        return (count == 0) ? 0 : total / count;
    }
};

// ----------- DRIVER -----------

int main() {
    Graph g(5);

    g.add_edge(0,1,1);
    g.add_edge(1,2,1);
    g.add_edge(2,3,1);
    g.add_edge(3,4,1);

    cout << "Degree of node 2: " << g.degree(2) << endl;
    cout << "Average degree: " << g.average_degree() << endl;
    cout << "Diameter: " << g.diameter() << endl;
    cout << "Avg path length: " << g.average_path_length() << endl;

    return 0;
}