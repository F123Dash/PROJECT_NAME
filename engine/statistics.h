#ifndef STATISTICS_H
#define STATISTICS_H

#include <vector>

class Statistics {
public:
    static double average(const std::vector<double>& data);
    static double variance(const std::vector<double>& data);
    static double minimum(const std::vector<double>& data);
    static double maximum(const std::vector<double>& data);
};

#endif
