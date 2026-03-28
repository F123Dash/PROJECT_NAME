#ifndef ROUTING_TABLE_H
#define ROUTING_TABLE_H

#include <vector>

std::vector<int> buildRoutingTable(
    std::vector<int>& parent,
    std::vector<int>& dist,
    int source,
    int n
);

void printRoutingTable(std::vector<int>& routing, int source);

#endif
