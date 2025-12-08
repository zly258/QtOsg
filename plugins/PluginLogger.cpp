#include "PluginLogger.h"
#include <cstdlib>

// Initialize static member
PluginLogger::LogLevel PluginLogger::currentLogLevel = PluginLogger::LOG_INFO;

void PluginLogger::setLogLevel(LogLevel level)
{
    currentLogLevel = level;
}

void PluginLogger::logError(const std::string &plugin, const std::string &message)
{
    log(LOG_ERROR, plugin, message);
}

void PluginLogger::logWarning(const std::string &plugin, const std::string &message)
{
    log(LOG_WARNING, plugin, message);
}

void PluginLogger::logInfo(const std::string &plugin, const std::string &message)
{
    log(LOG_INFO, plugin, message);
}

void PluginLogger::logDebug(const std::string &plugin, const std::string &message)
{
    log(LOG_DEBUG, plugin, message);
}

void PluginLogger::logPluginInit(const std::string &plugin, const std::string &version,
                                 const std::vector<std::string> &supportedFormats)
{
    std::ostringstream oss;
    oss << "Plugin initialized - Version: " << version << ", Supported formats: ";
    for (size_t i = 0; i < supportedFormats.size(); ++i)
    {
        if (i > 0)
            oss << ", ";
        oss << supportedFormats[i];
    }
    logInfo(plugin, oss.str());
}

void PluginLogger::logFileLoadStart(const std::string &plugin, const std::string &fileName)
{
    logInfo(plugin, "Loading file: " + fileName);
}

void PluginLogger::logFileLoadSuccess(const std::string &plugin, const std::string &fileName,
                                      long loadTimeMs, int nodeCount)
{
    std::ostringstream oss;
    oss << "Successfully loaded " << fileName << " in " << loadTimeMs << "ms";
    if (nodeCount >= 0)
    {
        oss << " (" << nodeCount << " nodes)";
    }
    logInfo(plugin, oss.str());
}

void PluginLogger::logFileLoadFailure(const std::string &plugin, const std::string &fileName,
                                      const std::string &errorMessage)
{
    logError(plugin, "Failed to load " + fileName + ": " + errorMessage);
}

void PluginLogger::logPluginCapabilities(const std::string &plugin, const std::vector<std::string> &capabilities)
{
    std::ostringstream oss;
    oss << "Plugin capabilities: ";
    for (size_t i = 0; i < capabilities.size(); ++i)
    {
        if (i > 0)
            oss << ", ";
        oss << capabilities[i];
    }
    logDebug(plugin, oss.str());
}

void PluginLogger::logSystemInfo(const std::string &plugin)
{
    // Log basic system information for debugging
    logDebug(plugin, "System information:");

    // Log compiler information
#ifdef _MSC_VER
    logDebug(plugin, "  Compiler: Microsoft Visual C++ " + std::to_string(_MSC_VER));
#elif defined(__GNUC__)
    logDebug(plugin, "  Compiler: GCC " + std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__));
#elif defined(__clang__)
    logDebug(plugin, "  Compiler: Clang " + std::to_string(__clang_major__) + "." + std::to_string(__clang_minor__));
#else
    logDebug(plugin, "  Compiler: Unknown");
#endif

    // Log platform information
#ifdef _WIN32
    logDebug(plugin, "  Platform: Windows");
#elif defined(__linux__)
    logDebug(plugin, "  Platform: Linux");
#elif defined(__APPLE__)
    logDebug(plugin, "  Platform: macOS");
#else
    logDebug(plugin, "  Platform: Unknown");
#endif

    // Log architecture
#ifdef _WIN64
    logDebug(plugin, "  Architecture: x64");
#elif defined(_WIN32)
    logDebug(plugin, "  Architecture: x86");
#elif defined(__x86_64__)
    logDebug(plugin, "  Architecture: x86_64");
#elif defined(__i386__)
    logDebug(plugin, "  Architecture: i386");
#else
    logDebug(plugin, "  Architecture: Unknown");
#endif

    // Log build configuration
#ifdef _DEBUG
    logDebug(plugin, "  Build: Debug");
#else
    logDebug(plugin, "  Build: Release");
#endif
}

std::string PluginLogger::getCurrentTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) %
              1000;

    std::ostringstream oss;

#ifdef _WIN32
    // Use thread-safe localtime_s on Windows
    std::tm timeinfo;
    if (localtime_s(&timeinfo, &time_t) == 0)
    {
        oss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
    }
    else
    {
        oss << "INVALID_TIME";
    }
#else
    // Use localtime_r on POSIX systems, or localtime with mutex protection
    std::tm timeinfo;
    if (localtime_r(&time_t, &timeinfo) != nullptr)
    {
        oss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
    }
    else
    {
        oss << "INVALID_TIME";
    }
#endif

    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

void PluginLogger::log(LogLevel level, const std::string &plugin, const std::string &message)
{
    if (level > currentLogLevel)
    {
        return;
    }

    std::ostream &stream = (level == LOG_ERROR) ? std::cerr : std::cout;
    stream << "[" << getCurrentTimestamp() << "] "
           << "[" << getLogLevelString(level) << "] "
           << "[" << plugin << "] "
           << message << std::endl;
}

std::string PluginLogger::getLogLevelString(LogLevel level)
{
    switch (level)
    {
    case LOG_ERROR:
        return "ERROR";
    case LOG_WARNING:
        return "WARN ";
    case LOG_INFO:
        return "INFO ";
    case LOG_DEBUG:
        return "DEBUG";
    default:
        return "UNKN ";
    }
}