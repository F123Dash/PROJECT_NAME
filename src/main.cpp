#include "../engine/pipeline.h"
#include <iostream>
#include <stdexcept>

int main() {
    try {
        runSimulationPipeline();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        return 1;
    }
}
