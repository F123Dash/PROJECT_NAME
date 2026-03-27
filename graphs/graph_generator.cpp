#include <iostream>
#include <random>
#include "graphs.h"
#include "graph_generator.h"

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

/*
 * Random Tree Generator
 * Creates a tree with n nodes by connecting each node i to a random parent
 * node with index < i. This guarantees connectivity and forms a valid tree.
 */
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

    // Ensure connectivity: node i connects to a random parent with index < i
    // This creates n-1 edges forming a connected acyclic graph (tree)
    for (int i = 1; i < n; ++i) {
        std::uniform_int_distribution<> parent_dist(0, i - 1);
        int parent = parent_dist(gen);
        int w = weight_dist(gen);
        g.add_edge(i, parent, w);
    }

    return g;
}

/*
 * Grid/Mesh Graph Generator
 * Creates a 2D grid graph with rows×cols nodes arranged in a rectangular mesh.
 * Node indexing: node at position (i,j) = i*cols + j (row-major order)
 */
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

    // Iterate through each grid position and connect to neighbors
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {

            // Convert 2D grid position to 1D node index
            int u = i * cols + j;

            // Connect to right neighbor (same row, next column)
            if (j + 1 < cols) {
                int v = i * cols + (j + 1);
                int w = weight_dist(gen);
                g.add_edge(u, v, w);
            }

            // Connect to down neighbor (next row, same column)
            if (i + 1 < rows) {
                int v = (i + 1) * cols + j;
                int w = weight_dist(gen);
                g.add_edge(u, v, w);
            }
        }
    }

    return g;
}


Graph generate_graph(
    GraphType type,
    int n,
    double p,
    int rows,
    int cols,
    int w_min,
    int w_max
) {
    if (w_min > w_max) {
        throw std::invalid_argument("Invalid weight range");
    }

    switch (type) {

        case GraphType::RANDOM:
            if (n <= 0 || p < 0 || p > 1)
                throw std::invalid_argument("Invalid RANDOM params");
            return generate_random_graph(n, p, w_min, w_max);

        case GraphType::TREE:
            if (n <= 0)
                throw std::invalid_argument("Invalid TREE params");
            return generate_tree_graph(n, w_min, w_max);

        case GraphType::GRID:
            if (rows <= 0 || cols <= 0)
                throw std::invalid_argument("Invalid GRID params");
            return generate_grid_graph(rows, cols, w_min, w_max);

        default:
            throw std::invalid_argument("Unknown graph type");
    }
}