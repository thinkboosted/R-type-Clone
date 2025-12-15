#include "Rtype.hpp"
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char **argv) {
  try {
    std::vector<std::string> appArgs;
    std::string programName = argv[0];
    size_t lastSlash = programName.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
      programName = programName.substr(lastSlash + 1);
    }

    std::string mode;
    if (programName == "r-type_server") {
      mode = "server";
    } else if (programName == "r-type_client") {
      mode = "client";
    } else {
      std::cerr << "Error: Unknown program name '" << programName
                << "'. Expected 'r-type_server' or 'r-type_client'."
                << std::endl;
      return 1;
    }
    appArgs.push_back(mode);

    for (int i = 1; i < argc; ++i) {
      appArgs.push_back(argv[i]);
    }

    if (mode == "server" && appArgs.size() < 2) {
      std::cerr
          << "Error: Server mode requires a port. Usage: r-type_server <port>"
          << std::endl;
      return 1;
    } else if (mode == "client" && appArgs.size() < 3) {
      std::cerr << "Error: Client mode requires IP and port. Usage: "
                   "r-type_client <server_ip> <server_port>"
                << std::endl;
      return 1;
    }

    // "127.0.0.1:0" is a placeholder; Rtype constructor handles the actual
    // internal bus configuration
    rtypeGame::Rtype app("127.0.0.1:0", appArgs);

    std::cout << "Starting Rtype application (" << mode << " mode)..."
              << std::endl;
    app.run();
    std::cout << "Rtype application has been closed successfully." << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
