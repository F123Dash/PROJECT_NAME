#include "../engine/simulator.h"
#include <iostream>
#include <stdexcept>

int main() {
    try {
        Simulator sim;

        sim.load_config("config/config.txt");
        sim.load_topology();
        sim.init_system();
        sim.init_traffic();

        sim.run();
        sim.finalize();

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        return 1;
    }
}
