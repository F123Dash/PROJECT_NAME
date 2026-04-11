#ifndef PIPELINE_H
#define PIPELINE_H

#include "simulator.h"

class Pipeline {
public:
    Simulator* sim;

    Pipeline(Simulator* s);

    void execute();
};

#endif