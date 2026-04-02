#include "simulation_clock.h"

int SimulationClock::current_time = 0;

int SimulationClock::getTime() {
    return current_time;
}

void SimulationClock::setTime(int time) {
    current_time = time;
}
