#include <iostream>
#include <vector>
#include "Rtype.hpp"

int main(int argc, char** argv) {
  try {
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    std::string zmqEndpoint = "127.0.0.1:5557";
    if (!args.empty() && args[0] == "client") {
        zmqEndpoint = "127.0.0.1:5559";
    }

    rtypeGame::Rtype app(zmqEndpoint, args);
    std::cout << "Starting Rtype application..." << std::endl;
    app.run();
    std::cout << "Rtype application has been closed successfully." << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
