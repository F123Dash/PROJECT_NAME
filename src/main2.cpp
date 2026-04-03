// Graph, MST, Shortest Path project issue 1
#include <iostream>
#include <fstream>
#include <vector>

#include "../graphs/graphs.h"
#include "mst.h"
#include "metrics.h"
#include "shortest_path.h"

using namespace std;

int main() {
    ifstream file("input.txt");

    if (!file.is_open()) {
        cerr << "Error: Could not open input file\n";
        return 1;
    }

    int n, m;
    file >> n >> m;

    Graph g(n);
    vector<Edge> edges;

    // Read edges
    for (int i = 0; i < m; i++) {
        int u, v, w;
        file >> u >> v >> w;

        g.add_edge(u, v, w);
        edges.push_back({u, v, w});
    }

    // Source & destination for testing
    int src = 0, dest = n - 1;

    // Shortest Path
    vector<int> sp = shortest_path(g, src, dest);
    PathMetrics sp_metrics = compute_path_metrics(sp, g);

    // MST (using Prim)
    vector<Edge> mst_edges = prim(g);

    Graph mst_graph(n);
    for (auto &e : mst_edges) {
        mst_graph.add_edge(e.u, e.v, e.w);
    }

    // MST Path
    vector<int> mst_p = mst_path(mst_graph, src, dest);
    PathMetrics mst_metrics = compute_path_metrics(mst_p, mst_graph);

    // Stretch
    double stretch = path_stretch(sp_metrics.path_cost, mst_metrics.path_cost);

    //OPs
    cout << "Shortest Path Cost: " << sp_metrics.path_cost << endl;
    cout << "MST Path Cost: " << mst_metrics.path_cost << endl;
    cout << "Path Stretch: " << stretch << endl;

    return 0;
}
