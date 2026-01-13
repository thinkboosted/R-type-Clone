#pragma once

#include <string>
#include <set>
#include <mutex>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <thread>
#include <sstream>

namespace rtypeEngine {

class Logger {
public:
    enum class Level {
        INFO,
        DEBUG,
        ERROR,
        TRAFFIC
    };

    static void Info(const std::string& message);
    static void Debug(const std::string& message);
    static void Error(const std::string& message);
    
    // Specifically for bus traffic logging with filtering
    static void LogTraffic(const std::string& direction, const std::string& source, const std::string& topic, const std::string& payload);

    static void setDebugEnabled(bool enabled);

private:
    static std::mutex _mutex;
    static bool _debugEnabled;
    static std::set<std::string> _ignoredTopics;

    static void Log(Level level, const std::string& message);
    static std::string getTimestamp();
    static std::string getThreadId();
    static std::string truncate(const std::string& str, size_t width);
};

} // namespace rtypeEngine
