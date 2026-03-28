#include <iostream>
#include <vector>
#include <queue>
#include <climits>
 
using namespace std;
 
// pair<int,int> = {weight, vertex} — used in priority queue
using pii = pair<int, int>;
 
const int INF = INT_MAX;
 
// ---------- PRINT PATH ----------
void printPath(vector<int>& parent, int j) 
{
    if (j == -1) return;
    printPath(parent, parent[j]);
    cout << j << " ";
}
 
// ---------- DIJKSTRA (with Priority Queue) ----------
// C manually scanned all nodes to find min — O(n²)
// C++ priority_queue (min-heap) does it in O(log n)
void dijkstra(vector<vector<pii>>& adj, int n, int source,vector<int>& dist, vector<int>& parent) 
{
 
    dist.assign(n, INF);       // replaces manual for-loop init
    parent.assign(n, -1);
 
    // Min-heap: {distance, vertex}
    priority_queue<pii, vector<pii>, greater<pii>> pq;
 
    dist[source] = 0;
    pq.push({0, source});
 
    while (!pq.empty()) {
        auto [d, u] = pq.top();   // structured binding (C++17)
        pq.pop();
 
        // Skip stale entries — replaces visited[] array
        if (d > dist[u]) continue;
 
        for (auto [w, v] : adj[u]) {
            if (dist[u] != INF && dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                parent[v] = u;
                pq.push({dist[v], v});
            }
        }
    }
}
 
// ---------- BELLMAN-FORD ----------
struct Edge 
{
    int u, v, w;
};
 
void bellmanFord(vector<Edge>& edges, int n, int source,vector<int>& dist, vector<int>& parent) 
{
    dist.assign(n, INF);
    parent.assign(n, -1);
    dist[source] = 0;
 
    for (int i = 0; i < n - 1; i++) {
        for (auto& e : edges) {           // range-based for (no index needed)
            if (dist[e.u] != INF && dist[e.u] + e.w < dist[e.v]) {
                dist[e.v] = dist[e.u] + e.w;
                parent[e.v] = e.u;
            }
        }
    }
 
    // Negative cycle check
    for (auto& e : edges) {
        if (dist[e.u] != INF && dist[e.u] + e.w < dist[e.v]) {
            cout << "Negative weight cycle detected!\n";
            return;
        }
    }
}
 
// ---------- ADD EDGE ----------
// adj[u] stores {weight, vertex} pairs — no manual Node struct/malloc needed
void addEdge(vector<vector<pii>>& adj, int u, int v, int w) {
    adj[u].push_back({w, v});   // replaces manual linked list + malloc
    adj[v].push_back({w, u});
}
 
// ---------- MAIN ----------
int main() {
    int n, m;
    cout << "Enter number of nodes: ";
    cin >> n;
 
    // vector replaces fixed-size arrays — dynamic, no MAX needed
    vector<vector<pii>> full_adj(n), mst_adj(n);
    vector<Edge> edges;
 
    cout << "Enter number of edges in FULL GRAPH: ";
    cin >> m;
 
    cout << "Enter edges (u v w):\n";
    for (int i = 0; i < m; i++) {
        int u, v, w;
        cin >> u >> v >> w;
        addEdge(full_adj, u, v, w);
        edges.push_back({u, v, w});   // replaces edges[edgeCount++]
    }
 
    int mstEdges;
    cout << "Enter number of edges in MST GRAPH: ";
    cin >> mstEdges;
 
    cout << "Enter edges (u v w) for MST:\n";
    for (int i = 0; i < mstEdges; i++) {
        int u, v, w;
        cin >> u >> v >> w;
        addEdge(mst_adj, u, v, w);
    }
 
    int source;
    cout << "Enter source node: ";
    cin >> source;
 
    vector<int> distFull, parentFull;
    vector<int> distMST,  parentMST;
    vector<int> distBF,   parentBF;
 
    dijkstra(full_adj, n, source, distFull, parentFull);
    dijkstra(mst_adj,  n, source, distMST,  parentMST);
    bellmanFord(edges, n, source, distBF,   parentBF);
 
    // Results table
    cout << "\nNODE | Dijkstra Full | Dijkstra MST | Bellman-Ford Full\n";
    cout << "------------------------------------------------------------\n";
    for (int i = 0; i < n; i++) {
        cout << i
             << " | " << (distFull[i] == INF ? -1 : distFull[i])
             << "             | " << (distMST[i] == INF ? -1 : distMST[i])
             << "            | " << (distBF[i]   == INF ? -1 : distBF[i])
             << "\n";
    }
 
    // Print paths
    cout << "\n--- Shortest Paths from source " << source << " (Dijkstra Full) ---\n";
    for (int i = 0; i < n; i++) {
        if (i == source) continue;
        cout << source << " -> " << i << " : ";
        if (distFull[i] == INF) cout << "No path";
        else printPath(parentFull, i);
        cout << "(cost: " << distFull[i] << ")\n";
    }
 
    return 0;
}
 
