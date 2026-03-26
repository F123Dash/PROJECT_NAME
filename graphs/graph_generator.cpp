#include <iostream>
#include <random>
#include "graphs.h"

/*
 * Erdős–Rényi Random Graph Generator G(n, p)
 * n = number of nodes
 * p = probability of edge between any pair (0 ≤ p ≤ 1)
 * weight range = [w_min, w_max]
 */

Graph generate_random_graph(int n, double p, int w_min = 1, int w_max = 10) {
    if (n <= 0) {
        throw std::invalid_argument("n must be positive");
    }
    if (p < 0.0 || p > 1.0) {
        throw std::invalid_argument("p must be in [0,1]");
    }
    if (w_min > w_max) {
        throw std::invalid_argument("Invalid weight range");
    }

    Graph g(n);

    // Random generators
    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_real_distribution<> prob_dist(0.0, 1.0);
    std::uniform_int_distribution<> weight_dist(w_min, w_max);

    // Iterate over all unordered pairs (u < v)
    for (int u = 0; u < n; ++u) {
        for (int v = u + 1; v < n; ++v) {

            double r = prob_dist(gen);

            if (r < p) {
                int w = weight_dist(gen);
                g.add_edge(u, v, w);
            }
        }
    }

    return g;
}