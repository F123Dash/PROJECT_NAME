#ifndef UNION_FIND_H
#define UNION_FIND_H

#include <vector>

class UnionFind {
private:
    std::vector<int> parent, rank;

public:
    UnionFind(int n);

    int find(int x);
    void unite(int x, int y);
};

#endif
