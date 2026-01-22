#include "logging.hpp"
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace Log {

namespace {
    std::ofstream g_logFile;
    bool g_initialized = false;
    std::mutex g_mutex;

    std::string timestamp() {
        std::time_t t = std::time(nullptr);
        std::tm *tm = std::gmtime(&t);
        if (!tm) return "[?]";
        std::ostringstream oss;
        oss << std::put_time(tm, "%Y-%m-%dT%H:%M:%SZ");
        return oss.str();
    }

    void write(const std::string &level, const std::string &msg) {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (!g_initialized || !g_logFile.is_open()) return;
        g_logFile << "[" << timestamp() << "] " << level << ": " << msg << "\n";
        g_logFile.flush();
    }
}

bool init(const std::string &filePath) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_logFile.is_open()) g_logFile.close();
    g_logFile.open(filePath, std::ios::out | std::ios::app);
    g_initialized = g_logFile.is_open();
    return g_initialized;
}

bool isInitialized() {
    return g_initialized;
}

void info(const std::string &msg) {
    write("INFO", msg);
}

void warn(const std::string &msg) {
    write("WARN", msg);
}

void error(const std::string &msg) {
    write("ERROR", msg);
}

} // namespace Log
