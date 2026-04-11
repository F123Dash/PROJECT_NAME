#include "../engine/simulator.h"
#include "../engine/pipeline.h"
#include <iostream>
#include <stdexcept>

int main() {
    try {
        Simulator sim;
        Pipeline pipeline(&sim);

        pipeline.execute();

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[MAIN] Fatal error: " << e.what() << std::endl;
        return 1;
    }
}