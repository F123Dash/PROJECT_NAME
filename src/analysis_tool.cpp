#include "../algorithms/mst.h"
#include "../algorithms/shortest_path.h"
#include "../algorithms/metrics.h"
#include "../graphs/graphs.h"
#include "../graphs/graph_analysis.h"
#include "../performance/profiler.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <climits>
#include <algorithm>

using namespace std;
using Clock = chrono::high_resolution_clock;

// Analysis Tool — uses existing C++ algorithms to compute MST, shortest path,
// complexity analysis, and MST vs SP comparison. Outputs JSON to stdout.

// Read topology_edges.json produced by the simulator
Graph load_topology_from_json(const string& filepath, int node_count) {
    Graph g(node_count);
    ifstream file(filepath);
    if (!file.is_open()) {
        cerr << "[ANALYSIS] Cannot open " << filepath << endl;
        return g;
    }

    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    // Simple JSON array parser for [{from:X, to:Y, weight:W}, ...]
    size_t pos = 0;
    while ((pos = content.find("\"from\"", pos)) != string::npos) {
        // Parse "from": X
        size_t colon = content.find(':', pos);
        size_t comma = content.find(',', colon);
        int from_node = stoi(content.substr(colon + 1, comma - colon - 1));

        // Parse "to": Y
        pos = content.find("\"to\"", comma);
        colon = content.find(':', pos);
        comma = content.find(',', colon);
        int to_node = stoi(content.substr(colon + 1, comma - colon - 1));

        // Parse "weight": W
        pos = content.find("\"weight\"", comma);
        colon = content.find(':', pos);
        size_t end = content.find_first_of(",}", colon);
        int weight = stoi(content.substr(colon + 1, end - colon - 1));

        g.add_edge(from_node, to_node, weight);
        pos = end;
    }

    return g;
}

string escape_json(const string& s) {
    string out;
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else out += c;
    }
    return out;
}

int main(int argc, char* argv[]) {
    // Usage: analysis_tool <node_count> <source> <destination> <topology_edges.json>
    if (argc < 5) {
        cerr << "Usage: analysis_tool <node_count> <source> <dest> <topology_json>" << endl;
        return 1;
    }

    int node_count = stoi(argv[1]);
    int source = stoi(argv[2]);
    int dest = stoi(argv[3]);
    string topo_file = argv[4];

    Graph g = load_topology_from_json(topo_file, node_count);

    int edge_count = 0;
    for (int i = 0; i < g.V; i++) edge_count += g.adj[i].size();
    edge_count /= 2;

    Profiler::enable();
    Profiler* profiler = Profiler::getInstance();
    profiler->reset();

    // ─── 1. Kruskal MST ───
    vector<Edge> edgeList;
    for (int u = 0; u < g.V; u++) {
        for (auto& [v, w] : g.adj[u]) {
            if (u < v) edgeList.push_back({u, v, w});
        }
    }

    auto t0 = Clock::now();
    profiler->start("kruskal");
    vector<Edge> kruskal_mst = kruskal(g.V, edgeList);
    profiler->stop("kruskal");
    double kruskal_time = chrono::duration<double, milli>(Clock::now() - t0).count();

    int kruskal_cost = 0;
    for (auto& e : kruskal_mst) kruskal_cost += e.w;

    // ─── 2. Prim MST ───
    t0 = Clock::now();
    profiler->start("prim");
    vector<Edge> prim_mst = prim(g);
    profiler->stop("prim");
    double prim_time = chrono::duration<double, milli>(Clock::now() - t0).count();

    int prim_cost = 0;
    for (auto& e : prim_mst) prim_cost += e.w;

    // ─── 3. Dijkstra ───
    vector<int> dijk_dist, dijk_parent;
    t0 = Clock::now();
    profiler->start("dijkstra");
    dijkstra(g, source, dijk_dist, dijk_parent);
    profiler->stop("dijkstra");
    double dijkstra_time = chrono::duration<double, milli>(Clock::now() - t0).count();

    // Reconstruct shortest path by simulating distributed hop-by-hop routing
    vector<int> sp_path;
    if (dest >= 0 && dest < node_count && dijk_dist[dest] != INT_MAX) {
        int curr = source;
        sp_path.push_back(curr);
        int hops = 0;
        
        while (curr != dest && hops < node_count) {
            vector<int> curr_dist, curr_parent;
            dijkstra(g, curr, curr_dist, curr_parent);
            
            if (curr_dist[dest] == INT_MAX) break;
            
            int next_hop = dest;
            while (curr_parent[next_hop] != curr && curr_parent[next_hop] != -1) {
                next_hop = curr_parent[next_hop];
            }
            
            if (next_hop == curr) break; // Safety fallback
            
            curr = next_hop;
            sp_path.push_back(curr);
            hops++;
        }
    }
    int sp_cost = (dest >= 0 && dest < node_count) ? dijk_dist[dest] : INT_MAX;

    // ─── 4. Bellman-Ford ───
    vector<int> bf_dist, bf_parent;
    t0 = Clock::now();
    profiler->start("bellman_ford");
    bool bf_ok = bellmanFord(g, source, bf_dist, bf_parent);
    profiler->stop("bellman_ford");
    double bf_time = chrono::duration<double, milli>(Clock::now() - t0).count();

    // ─── 5. BFS ───
    t0 = Clock::now();
    profiler->start("bfs");
    vector<int> bfs_dist = bfs(g, source);
    profiler->stop("bfs");
    double bfs_time = chrono::duration<double, milli>(Clock::now() - t0).count();

    // ─── 6. MST path + path stretch (using user's metrics.cpp functions) ───
    // Build MST as a Graph for mst_path()
    Graph mst_graph(node_count);
    for (auto& e : kruskal_mst) {
        mst_graph.add_edge(e.u, e.v, e.w);
    }

    vector<int> mst_p = mst_path(mst_graph, source, dest);
    PathMetrics mst_pm = compute_path_metrics(mst_p, mst_graph);
    PathMetrics sp_pm = compute_path_metrics(sp_path, g);

    double stretch = path_stretch(sp_pm.path_cost, mst_pm.path_cost);

    // ─── Output JSON ───
    cout << "{" << endl;

    // MST results
    cout << "  \"kruskal\": {" << endl;
    cout << "    \"totalCost\": " << kruskal_cost << "," << endl;
    cout << "    \"edgeCount\": " << kruskal_mst.size() << "," << endl;
    cout << "    \"time_ms\": " << kruskal_time << "," << endl;
    cout << "    \"edges\": [";
    for (size_t i = 0; i < kruskal_mst.size(); i++) {
        if (i > 0) cout << ",";
        cout << "{\"u\":" << kruskal_mst[i].u << ",\"v\":" << kruskal_mst[i].v << ",\"w\":" << kruskal_mst[i].w << "}";
    }
    cout << "]" << endl;
    cout << "  }," << endl;

    cout << "  \"prim\": {" << endl;
    cout << "    \"totalCost\": " << prim_cost << "," << endl;
    cout << "    \"edgeCount\": " << prim_mst.size() << "," << endl;
    cout << "    \"time_ms\": " << prim_time << "," << endl;
    cout << "    \"edges\": [";
    for (size_t i = 0; i < prim_mst.size(); i++) {
        if (i > 0) cout << ",";
        cout << "{\"u\":" << prim_mst[i].u << ",\"v\":" << prim_mst[i].v << ",\"w\":" << prim_mst[i].w << "}";
    }
    cout << "]" << endl;
    cout << "  }," << endl;

    // Shortest path results
    cout << "  \"shortest_path\": {" << endl;
    cout << "    \"cost\": " << (sp_cost == INT_MAX ? -1 : sp_cost) << "," << endl;
    cout << "    \"hops\": " << (sp_path.empty() ? 0 : (int)sp_path.size() - 1) << "," << endl;
    cout << "    \"path\": \"";
    for (size_t i = 0; i < sp_path.size(); i++) {
        if (i > 0) cout << " -> ";
        cout << sp_path[i];
    }
    cout << "\"," << endl;
    cout << "    \"dijkstra_time_ms\": " << dijkstra_time << endl;
    cout << "  }," << endl;

    // MST path comparison
    cout << "  \"mst_path\": {" << endl;
    cout << "    \"cost\": " << mst_pm.path_cost << "," << endl;
    cout << "    \"hops\": " << mst_pm.path_length << "," << endl;
    cout << "    \"path\": \"";
    for (size_t i = 0; i < mst_p.size(); i++) {
        if (i > 0) cout << " -> ";
        cout << mst_p[i];
    }
    cout << "\"," << endl;
    cout << "    \"path_stretch\": " << stretch << endl;
    cout << "  }," << endl;

    // Complexity analysis
    cout << "  \"complexity\": {" << endl;
    cout << "    \"V\": " << node_count << "," << endl;
    cout << "    \"E\": " << edge_count << "," << endl;
    cout << "    \"results\": [" << endl;
    cout << "      {\"name\":\"Dijkstra\",\"bigO\":\"O((V+E) log V)\",\"measured_ms\":" << dijkstra_time << "}," << endl;
    cout << "      {\"name\":\"Bellman-Ford\",\"bigO\":\"O(V * E)\",\"measured_ms\":" << bf_time << "}," << endl;
    cout << "      {\"name\":\"BFS\",\"bigO\":\"O(V + E)\",\"measured_ms\":" << bfs_time << "}," << endl;
    cout << "      {\"name\":\"Kruskal MST\",\"bigO\":\"O(E log E)\",\"measured_ms\":" << kruskal_time << "}," << endl;
    cout << "      {\"name\":\"Prim MST\",\"bigO\":\"O(E log V)\",\"measured_ms\":" << prim_time << "}" << endl;
    cout << "    ]" << endl;
    cout << "  }," << endl;

    cout << "  \"bellman_ford_no_neg_cycle\": " << (bf_ok ? "true" : "false") << "," << endl;
    cout << "  \"bfs_distance\": " << (dest >= 0 && dest < node_count ? bfs_dist[dest] : -1) << endl;
    cout << "}" << endl;

    Profiler::disable();
    Profiler::destroyInstance();

    return 0;
}
