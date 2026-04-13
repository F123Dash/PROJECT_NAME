#ifndef PIPELINE_H
#define PIPELINE_H

#include "simulator.h"
#include "data_collector.h"

class Pipeline {
public:
    Simulator* sim;
    DataCollector* data_collector;

    Pipeline(Simulator* s, DataCollector* dc = nullptr);

    void execute();
};

#endif