#include <iostream>
#include <fstream>
#include <vector>
#include <tuple>
#include <queue>
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

int main() {
    string filename;
    cout << "Enter file name: ";
    cin >> filename;

    try {
        Graph g = load_graph(filename);

        cout << "\nGraph Analysis:\n";

        for (int i = 0; i < g.V; i++) {
            cout << "Degree of node " << i << ": " << g.degree(i) << endl;
        }

        cout << "Average degree: " << g.average_degree() << endl;
        cout << "Diameter: " << g.diameter() << endl;
        cout << "Average path length: " << g.average_path_length() << endl;

    } catch (exception &e) {
        cout << e.what() << endl;
    }

    return 0;
}