#include "routing_table.h"
#include <iostream>
#include <climits>

using namespace std;

const int INF = INT_MAX;

vector<int> buildRoutingTable(vector<int>& parent, vector<int>& dist, int source, int n) {
    vector<int> routing(n, -1);

    for (int i = 0; i < n; i++) {

        if (i == source) {
            routing[i] = source;
        }
        else if (dist[i] == INF) {
            routing[i] = -1;
        }
        else {
            int curr = i;

            while (curr != -1) {
                if (parent[curr] == source) break;
                if (parent[curr] == -1) {
                    curr = -1;
                    break;
                }
                curr = parent[curr];
            }

            routing[i] = (curr == -1) ? -1 : curr;
        }
    }

    return routing;
}

void printRoutingTable(vector<int>& routing, int source) {
    cout << "\nRouting Table (Source " << source << ")\n";
    cout << "Destination -> Next Hop\n";
    cout << "------------------------\n";

    for (int i = 0; i < routing.size(); i++) {
        if (routing[i] == -1)
            cout << i << " -> No Path\n";
        else
            cout << i << " -> " << routing[i] << "\n";
    }
}
