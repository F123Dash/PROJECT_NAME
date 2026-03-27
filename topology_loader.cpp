#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <limits>

using namespace std;

struct Edge {
    int to;
    double weight;
};

class Graph {
public:
    unordered_map<int, vector<Edge>> adj;

    void add_edge(int u, int v, double w) {
        adj[u].push_back({v, w});
        adj[v].push_back({u, w}); // remove this if directed
    }

    void print() const {
        for (const auto &pair : adj) {
            cout << "Node " << pair.first << ": ";
            for (const auto &edge : pair.second) {
                cout << "(" << edge.to << ", " << edge.weight << ") ";
            }
            cout << endl;
        }
    }
};

bool is_valid_integer(const string &s) {
    try {
        size_t pos;
        stoi(s, &pos);
        return pos == s.size();
    } catch (...) {
        return false;
    }
}

bool is_valid_double(const string &s) {
    try {
        size_t pos;
        stod(s, &pos);
        return pos == s.size();
    } catch (...) {
        return false;
    }
}

Graph load_graph(const string &filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("Error: Cannot open file.");
    }

    Graph g;
    string line;
    int line_num = 0;

    while (getline(file, line)) {
        line_num++;

        // Skip empty lines
        if (line.empty()) continue;

        stringstream ss(line);
        string n1, n2, w;

        if (!(ss >> n1 >> n2 >> w)) {
            throw runtime_error("Malformed line at line " + to_string(line_num));
        }

        // Validation
        if (!is_valid_integer(n1) || !is_valid_integer(n2)) {
            throw runtime_error("Invalid node index at line " + to_string(line_num));
        }

        if (!is_valid_double(w)) {
            throw runtime_error("Invalid weight at line " + to_string(line_num));
        }

        int node1 = stoi(n1);
        int node2 = stoi(n2);
        double weight = stod(w);

        // Optional: restrict node indices (e.g., non-negative)
        if (node1 < 0 || node2 < 0) {
            throw runtime_error("Negative node index at line " + to_string(line_num));
        }

        g.add_edge(node1, node2, weight);
    }

    file.close();
    return g;
}

int main() {
    string filename;
    cout << "Enter file name: ";
    cin >> filename;

    try {
        Graph g = load_graph(filename);
        cout << "Graph loaded successfully:\n";
        g.print();
    } catch (const exception &e) {
        cerr << e.what() << endl;
    }

    return 0;
}