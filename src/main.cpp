#include <iostream>
#include "graph.h"
#include "shortest_path.h"
#include "routing_table.h"

using namespace std;

void printArray(vector<int>& arr, string name) {
    cout << "\n" << name << ":\n";
    for (int i = 0; i < arr.size(); i++) {
        cout << i << " : " << arr[i] << "\n";
    }
}

int main() {
    int n, m;
    cin >> n >> m;

    Graph g(n);

    for (int i = 0; i < m; i++) {
        int u, v, w;
        cin >> u >> v >> w;
        g.add_edge(u, v, w);
    }

    int source;
    cin >> source;

    vector<int> dist, parent;

    // Phase 3
    dijkstra(g, source, dist, parent);

    // 🔹 PRINT DISTANCE + PARENT
    printArray(dist, "Distance Array");
    printArray(parent, "Parent Array");

    // Phase 4
    vector<int> routing = buildRoutingTable(parent, dist, source, n);

    printRoutingTable(routing, source);

    return 0;
}
