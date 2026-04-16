#include <iostream>
#include <vector>
#include "../graphs/graphs.h"
#include "../algorithms/metrics.h"

int main() {
    Graph g(3);
    g.add_edge(0, 1, 5);
    g.add_edge(1, 2, 7);

    std::vector<int> empty_path;
    PathMetrics m0 = compute_path_metrics(empty_path, g);
    std::cout << "empty_path_length=" << m0.path_length << " empty_path_cost=" << m0.path_cost << "\n";

    std::vector<int> valid_path = {0, 1, 2};
    PathMetrics m1 = compute_path_metrics(valid_path, g);
    std::cout << "valid_path_length=" << m1.path_length << " valid_path_cost=" << m1.path_cost << "\n";

    std::cout << "stretch_zero_shortest=" << path_stretch(0, 12) << "\n";
    return 0;
}
