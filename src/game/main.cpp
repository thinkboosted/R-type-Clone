#include <iostream>
#include "Rtype.hpp"

int main(void) {
  try {
    rtypeGame::Rtype app("127.0.0.1:5557");
    std::cout << "Starting Rtype application..." << std::endl;
    app.run();
    std::cout << "Rtype application has been closed successfully." << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
