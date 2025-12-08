#ifndef PLUGINLOGGER_H
#define PLUGINLOGGER_H

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>

/**
 * @brief Centralized logging utility for OSG plugins
 * 
 * Provides consistent logging functionality across all plugins with different log levels
 * and formatted output including timestamps and plugin identification.
 */
class PluginLogger {
public:
    enum LogLevel {
        LOG_ERROR = 0,
        LOG_WARNING = 1,
        LOG_INFO = 2,
        LOG_DEBUG = 3
    };

    /**
     * @brief Set global log level
     * @param level Log level threshold
     */
    static void setLogLevel(LogLevel level);

    /**
     * @brief Log error message
     * @param plugin Plugin name
     * @param message Error message
     */
    static void logError(const std::string& plugin, const std::string& message);

    /**
     * @brief Log warning message
     * @param plugin Plugin name
     * @param message Warning message
     */
    static void logWarning(const std::string& plugin, const std::string& message);

    /**
     * @brief Log info message
     * @param plugin Plugin name
     * @param message Info message
     */
    static void logInfo(const std::string& plugin, const std::string& message);

    /**
     * @brief Log debug message
     * @param plugin Plugin name
     * @param message Debug message
     */
    static void logDebug(const std::string& plugin, const std::string& message);

    /**
     * @brief Log plugin initialization
     * @param plugin Plugin name
     * @param version Plugin version
     * @param supportedFormats Supported file formats
     */
    static void logPluginInit(const std::string& plugin, const std::string& version, 
                             const std::vector<std::string>& supportedFormats);

    /**
     * @brief Log plugin capabilities and features
     * @param plugin Plugin name
     * @param capabilities List of plugin capabilities
     */
    static void logPluginCapabilities(const std::string& plugin, const std::vector<std::string>& capabilities);

    /**
     * @brief Log system information for debugging
     * @param plugin Plugin name
     */
    static void logSystemInfo(const std::string& plugin);

    /**
     * @brief Log file loading start
     * @param plugin Plugin name
     * @param fileName File name being loaded
     */
    static void logFileLoadStart(const std::string& plugin, const std::string& fileName);

    /**
     * @brief Log file loading success
     * @param plugin Plugin name
     * @param fileName File name loaded
     * @param loadTimeMs Loading time in milliseconds
     * @param nodeCount Number of nodes loaded
     */
    static void logFileLoadSuccess(const std::string& plugin, const std::string& fileName, 
                                  long loadTimeMs, int nodeCount = -1);

    /**
     * @brief Log file loading failure
     * @param plugin Plugin name
     * @param fileName File name that failed to load
     * @param errorMessage Error message
     */
    static void logFileLoadFailure(const std::string& plugin, const std::string& fileName, 
                                  const std::string& errorMessage);

private:
    static LogLevel currentLogLevel;
    
    /**
     * @brief Get current timestamp string
     * @return Formatted timestamp
     */
    static std::string getCurrentTimestamp();

    /**
     * @brief Log message with level
     * @param level Log level
     * @param plugin Plugin name
     * @param message Message
     */
    static void log(LogLevel level, const std::string& plugin, const std::string& message);

    /**
     * @brief Get log level string
     * @param level Log level
     * @return Log level string
     */
    static std::string getLogLevelString(LogLevel level);
};

#endif // PLUGINLOGGER_H