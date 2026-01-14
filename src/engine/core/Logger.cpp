#include "Logger.hpp"

namespace rtypeEngine {

std::mutex Logger::_mutex;
bool Logger::_debugEnabled = false;

// Default Blacklist - Add noisy topics here
std::set<std::string> Logger::_ignoredTopics = {
    "MouseMoved",
    "ImageRendered", 
    "RenderEntityCommand", 
    "Heartbeat",
    "FrameMetrics",
    "PipelinePhase"
};

void Logger::setDebugEnabled(bool enabled) {
    _debugEnabled = enabled;
}

void Logger::Info(const std::string& message) {
    Log(Level::INFO, message);
}

void Logger::Debug(const std::string& message) {
    if (_debugEnabled) {
        Log(Level::DEBUG, message);
    }
}

void Logger::Error(const std::string& message) {
    Log(Level::ERROR, message);
}

void Logger::LogTraffic(const std::string& direction, const std::string& source, const std::string& topic, const std::string& payload) {
    if (!_debugEnabled) return;

    // Filter out ignored topics
    if (_ignoredTopics.find(topic) != _ignoredTopics.end()) {
        return;
    }

    bool isBinary = (payload.size() > 200);
    std::stringstream ss;
    ss << direction << " [" << source << "] " << topic << " | ";
    
    if (isBinary) {
        ss << "[Binary/Large Payload: " << payload.size() << " bytes]";
    } else {
        ss << payload;
    }

    Log(Level::TRAFFIC, ss.str());
}

void Logger::Log(Level level, const std::string& message) {
    std::lock_guard<std::mutex> lock(_mutex);

    std::string levelStr;
    std::string colorCode;

    switch (level) {
        case Level::INFO:    levelStr = "INFO "; colorCode = "\033[32m"; break; // Green
        case Level::DEBUG:   levelStr = "DEBUG"; colorCode = "\033[36m"; break; // Cyan
        case Level::ERROR:   levelStr = "ERROR"; colorCode = "\033[31m"; break; // Red
        case Level::TRAFFIC: levelStr = "BUS  "; colorCode = "\033[35m"; break; // Magenta
    }

    std::cout << colorCode 
              << "[" << getTimestamp() << "]"
              << "[" << getThreadId() << "]"
              << "[" << levelStr << "] " 
              << message 
              << "\033[0m" << std::endl;
}

std::string Logger::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    auto timer = std::chrono::system_clock::to_time_t(now);
    std::tm bt = *std::localtime(&timer);

    std::stringstream ss;
    ss << std::put_time(&bt, "%H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string Logger::getThreadId() {
    std::stringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
}

std::string Logger::truncate(const std::string& str, size_t width) {
    if (str.length() > width) {
        return str.substr(0, width) + "...";
    }
    return str;
}

} // namespace rtypeEngine
