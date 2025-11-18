#ifndef PLATFORM_H
#define PLATFORM_H

class HexagonApp;

namespace Platform {
    void initialize(int argc, char** argv);
    void run(HexagonApp& app);
}

#endif // PLATFORM_H
