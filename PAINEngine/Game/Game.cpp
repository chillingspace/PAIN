#include <iostream>
#include "Engine.h"

int main() {
    Engine engine;
    engine.Hello();  // Calls into the DLL
    std::cin.get();  // Pause so console stays open
    return 0;
}
