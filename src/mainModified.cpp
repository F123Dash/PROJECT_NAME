#include <iostream>
#include <vector>
#include "graph.h"
#include "shortest_path.h"
#include "routing_table.h"
#include "event.h"

using namespace std;

// 🔹 Helper: print array
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

    // ===============================
    // 🔹 Phase 3: Shortest Path
    // ===============================
    vector<int> dist, parent;

    dijkstra(g, source, dist, parent);

    printArray(dist, "Distance Array");
    printArray(parent, "Parent Array");

    // ===============================
    // 🔹 Phase 4: Routing Table
    // ===============================
    vector<int> routing = buildRoutingTable(parent, dist, source, n);

    printRoutingTable(routing, source);

    // ===============================
    // 🔹 Phase 1: Event System
    // ===============================
    cout << "\nEvent Simulation:\n";

    // 🔹 simple local event queue
    priority_queue<Event, vector<Event>, greater<Event>> eq;

    int destination = n - 1;

    Event e;
    e.time = 0;
    e.type = "PACKET_SEND";

    // ✅ Correct: use routing table
    e.callback = [source, destination, parent]() {
    cout << "Packet path: ";

    vector<int> path;
    int current = destination;

    // build path backwards
    while (current != -1) {
        path.push_back(current);
        current = parent[current];
    }

    // reverse path
    reverse(path.begin(), path.end());

    // print path
    for (int i = 0; i < path.size(); i++) {
        cout << path[i];
        if (i != path.size() - 1)
            cout << " -> ";
    }

    cout << endl;
};

    eq.push(e);

    while (!eq.empty()) {
        Event curr = eq.top();
        eq.pop();
        curr.callback();
    }

    return 0;
}
