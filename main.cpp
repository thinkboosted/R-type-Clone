#include "app.h"
#include "platform.h"

int main(int argc, char** argv) {
    Platform::initialize(argc, argv);

    HexagonApp app;
    Platform::run(app);

    return 0;
}
