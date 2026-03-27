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

Graph generate_tree_graph(int n, int w_min = 1, int w_max = 10) {
    if (n <= 0) {
        throw std::invalid_argument("n must be positive");
    }
    if (w_min > w_max) {
        throw std::invalid_argument("Invalid weight range");
    }

    Graph g(n);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> weight_dist(w_min, w_max);

    // Ensure connectivity: node i connects to any node < i
    for (int i = 1; i < n; ++i) {
        std::uniform_int_distribution<> parent_dist(0, i - 1);
        int parent = parent_dist(gen);
        int w = weight_dist(gen);
        g.add_edge(i, parent, w);
    }

    return g;
}

Graph generate_grid_graph(int rows, int cols, int w_min = 1, int w_max = 10) {
    if (rows <= 0 || cols <= 0) {
        throw std::invalid_argument("Invalid grid size");
    }
    if (w_min > w_max) {
        throw std::invalid_argument("Invalid weight range");
    }

    int n = rows * cols;
    Graph g(n);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> weight_dist(w_min, w_max);

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {

            int u = i * cols + j;

            // Right neighbor
            if (j + 1 < cols) {
                int v = i * cols + (j + 1);
                int w = weight_dist(gen);
                g.add_edge(u, v, w);
            }

            // Down neighbor
            if (i + 1 < rows) {
                int v = (i + 1) * cols + j;
                int w = weight_dist(gen);
                g.add_edge(u, v, w);
            }
        }
    }

    return g;
}
