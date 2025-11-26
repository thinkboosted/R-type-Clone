#include "BasicECSSavesManager.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <thread>

namespace fs = std::filesystem;

namespace rtypeEngine {

    ECSSavesManager::ECSSavesManager(const char* pubEndpoint, const char* subEndpoint)
        : IECSSavesManager(pubEndpoint, subEndpoint) {
    }

    ECSSavesManager::~ECSSavesManager() {
    }

    void ECSSavesManager::init() {
        subscribe("CreateSaveCommand", [this](const std::string& msg) {
            // msg format: saveName:data
            size_t split = msg.find(':');
            if (split != std::string::npos) {
                std::string saveName = msg.substr(0, split);
                std::string data = msg.substr(split + 1);
                createSave(saveName, data);
            }
        });

        subscribe("LoadLastSaveCommand", [this](const std::string& msg) {
            loadLastSave(msg);
        });

        subscribe("LoadFirstSaveCommand", [this](const std::string& msg) {
            loadFirstSave(msg);
        });

        subscribe("GetSaves", [this](const std::string& msg) {
            getSaves(msg);
        });

        std::cout << "[ECSSavesManager] Initialized" << std::endl;
    }

    void ECSSavesManager::loop() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void ECSSavesManager::cleanup() {
    }

    std::string ECSSavesManager::getTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);

        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y%m%d%H%M%S");
        return ss.str();
    }

    void ECSSavesManager::createSave(const std::string& saveName, const std::string& data) {
        std::string dirPath = "saves/" + saveName;
        if (!fs::exists(dirPath)) {
            fs::create_directories(dirPath);
        }

        std::string filename = dirPath + "/" + getTimestamp() + ".sv";
        std::ofstream outfile(filename);
        if (outfile.is_open()) {
            outfile << data;
            outfile.close();
            std::cout << "[ECSSavesManager] Saved to " << filename << std::endl;
        } else {
            std::cerr << "[ECSSavesManager] Failed to open file " << filename << std::endl;
        }
    }

    void ECSSavesManager::loadLastSave(const std::string& saveName) {
        std::string dirPath = "saves/" + saveName;
        if (!fs::exists(dirPath)) {
            std::cerr << "[ECSSavesManager] Save directory not found: " << dirPath << std::endl;
            return;
        }

        std::string lastFile;
        for (const auto& entry : fs::directory_iterator(dirPath)) {
            if (entry.path().extension() == ".sv") {
                if (lastFile.empty() || entry.path().string() > lastFile) {
                    lastFile = entry.path().string();
                }
            }
        }

        if (!lastFile.empty()) {
            std::ifstream infile(lastFile);
            if (infile.is_open()) {
                std::stringstream buffer;
                buffer << infile.rdbuf();
                sendMessage("ECSStateLoadedEvent", buffer.str());
                std::cout << "[ECSSavesManager] Loaded " << lastFile << std::endl;
            }
        }
    }

    void ECSSavesManager::loadFirstSave(const std::string& saveName) {
        std::string dirPath = "saves/" + saveName;
        if (!fs::exists(dirPath)) {
            std::cerr << "[ECSSavesManager] Save directory not found: " << dirPath << std::endl;
            return;
        }

        std::string firstFile;
        for (const auto& entry : fs::directory_iterator(dirPath)) {
            if (entry.path().extension() == ".sv") {
                if (firstFile.empty() || entry.path().string() < firstFile) {
                    firstFile = entry.path().string();
                }
            }
        }

        if (!firstFile.empty()) {
            std::ifstream infile(firstFile);
            if (infile.is_open()) {
                std::stringstream buffer;
                buffer << infile.rdbuf();
                sendMessage("ECSStateLoadedEvent", buffer.str());
                std::cout << "[ECSSavesManager] Loaded " << firstFile << std::endl;
            }
        }
    }

    void ECSSavesManager::getSaves(const std::string& saveName) {
        std::string dirPath = "saves/" + saveName;
        std::string savesList;
        if (fs::exists(dirPath)) {
            for (const auto& entry : fs::directory_iterator(dirPath)) {
                if (entry.path().extension() == ".sv") {
                    savesList += entry.path().filename().string() + ";";
                }
            }
        }
        sendMessage("SavesListEvent", savesList);
    }

}

extern "C" __declspec(dllexport) rtypeEngine::IModule* createModule(const char* pubEndpoint, const char* subEndpoint) {
    return new rtypeEngine::ECSSavesManager(pubEndpoint, subEndpoint);
}
