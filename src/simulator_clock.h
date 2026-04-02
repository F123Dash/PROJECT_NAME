#ifndef SIMULATION_CLOCK_H
#define SIMULATION_CLOCK_H

class SimulationClock {
private:
    static int current_time;

public:
    static int getTime();
    static void setTime(int time);
};

#endif
