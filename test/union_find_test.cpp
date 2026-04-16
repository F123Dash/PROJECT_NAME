#include <iostream>
#include "../algorithms/union_find.h"

int main() {
    const int n = 20000;
    UnionFind uf(n);

    for (int i = 1; i < n; ++i) {
        uf.unite(i - 1, i);
    }

    int root_last = uf.find(n - 1);
    int root_zero = uf.find(0);
    std::cout << "same_component=" << (root_last == root_zero ? 1 : 0) << "\n";
    return 0;
}
