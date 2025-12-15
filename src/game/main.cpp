#include "Rtype.hpp"
#include <algorithm> // Added for std::transform
#include <iostream>
#include <string> // Added for std::string operations
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
    appArgs.push_back(mode); // First argument is the mode

    // Add remaining command line arguments after the mode
    for (int i = 1; i < argc; ++i) {
      appArgs.push_back(argv[i]);
    }

    std::string zmqEndpoint = "127.0.0.1:5557"; // Default for local bus if no
                                                // specific port is derived
    if (mode == "server" && appArgs.size() >= 2) {
      // For server, use the provided port for the ZMQ bus as well (e.g., 4242)
      zmqEndpoint = "127.0.0.1:" + appArgs[1];
    } else if (mode == "client" && appArgs.size() >= 3) {
      // For client, the first arg in appArgs is "client", so endpoint is at
      // appArgs[1] This endpoint will be used by NetworkManager to connect to
      // the game server. We do NOT use this for the local ZMQ bus, as it will
      // use ephemeral ports (127.0.0.1:0). The zmqEndpoint for the local bus is
      // handled internally by Rtype's constructor now. We only care about the
      // target game server's address for NetworkManager.
    } else if (mode == "server" && appArgs.size() < 2) {
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

    // The zmqEndpoint in Rtype's constructor is for the internal message bus
    // endpoints. For clients, Rtype constructor will internally generate
    // ephemeral ports for its local bus. For server, it uses the provided port
    // as base. For client, the `endpoint` argument passed to Rtype's
    // constructor for its ZMQ bus should be "127.0.0.1:0" to signal ephemeral
    // ports. This is handled inside Rtype's constructor now. The `zmqEndpoint`
    // variable here is only used by this main.cpp directly if it were to
    // configure AApplication's broker. Rtype constructor now correctly
    // determines its own internal ZMQ bus endpoints.

    // Rtype constructor now directly takes the originally passed endpoint and
    // args. The logic to decide the internal ZMQ bus endpoints based on
    // client/server role is now handled inside Rtype's constructor.

    rtypeGame::Rtype app(
        "127.0.0.1:0",
        appArgs); // Pass "127.0.0.1:0" as a placeholder for the local ZMQ bus
                  // base endpoint for clients Rtype constructor will interpret
                  // this as a request for ephemeral ports For server, Rtype
                  // constructor will use appArgs[1] as the base port.

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
