#include "statistics.h"
#include <numeric>
#include <algorithm>
#include <cmath>

double Statistics::average(const std::vector<double>& data) {
    if (data.empty()) return 0.0;

    double sum = std::accumulate(data.begin(), data.end(), 0.0);
    return sum / data.size();
}

double Statistics::variance(const std::vector<double>& data) {
    if (data.empty()) return 0.0;

    double avg = average(data);
    double var = 0.0;

    for (double x : data) {
        var += (x - avg) * (x - avg);
    }

    return var / data.size();
}

double Statistics::minimum(const std::vector<double>& data) {
    if (data.empty()) return 0.0;

    return *std::min_element(data.begin(), data.end());
}

double Statistics::maximum(const std::vector<double>& data) {
    if (data.empty()) return 0.0;

    return *std::max_element(data.begin(), data.end());
}
