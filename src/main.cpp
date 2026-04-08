#include "../engine/simulator.h"

int main() {
    Simulator sim;

    sim.load_topology();
    sim.init_system();
    sim.init_traffic();

    sim.run();
    sim.finalize();

    return 0;
}
