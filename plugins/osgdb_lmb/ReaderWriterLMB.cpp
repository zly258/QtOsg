#include "ReaderWriterLMB.h"
#include "LmbParser.h"
#include "../PluginLogger.h"
#include <osgDB/FileNameUtils>
#include <osgDB/Registry>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>

ReaderWriterLMB::ReaderWriterLMB()
{
    supportsExtension("lmb", "LMB model format");

    // Check for debug mode environment variable
    const char *debugEnv = std::getenv("OSG_PLUGIN_DEBUG");
    if (debugEnv && (std::string(debugEnv) == "1" || std::string(debugEnv) == "true"))
    {
        PluginLogger::setLogLevel(PluginLogger::LOG_DEBUG);
    }

    // Log plugin initialization
    std::vector<std::string> formats = {"lmb"};
    PluginLogger::logPluginInit("LMB", "1.0.0", formats);

    // Log additional initialization details in debug mode
    PluginLogger::logDebug("LMB", "Plugin constructor called");

    // Log plugin capabilities
    std::vector<std::string> capabilities = {
        "Binary format parsing",
        "Progress callbacks",
        "Comprehensive error handling",
        "Vertex compression support",
        "Instance rendering support",
        "Material color mapping"};
    PluginLogger::logPluginCapabilities("LMB", capabilities);

    // Log system information in debug mode
    PluginLogger::logSystemInfo("LMB");

    PluginLogger::logDebug("LMB", "Plugin loaded successfully - ready to handle LMB files");
}

const char *ReaderWriterLMB::className() const
{
    return "LMB Reader/Writer";
}

osgDB::ReaderWriter::ReadResult ReaderWriterLMB::readNode(const std::string &fileName, const osgDB::Options *options) const
{
    std::string ext = osgDB::getLowerCaseFileExtension(fileName);
    if (!acceptsExtension(ext))
    {
        PluginLogger::logDebug("LMB", "File extension not supported: " + ext);
        return ReadResult::FILE_NOT_HANDLED;
    }

    std::string localFileName = fileName;

    // Check if file exists
    std::ifstream testFile(localFileName);
    if (!testFile.good())
    {
        PluginLogger::logError("LMB", "File not found: " + fileName);
        return ReadResult::FILE_NOT_FOUND;
    }
    testFile.close();

    auto startTime = std::chrono::high_resolution_clock::now();
    PluginLogger::logInfo("LMB", "Starting to load file: " + localFileName);

    try
    {
        // Extract progress callback from OSG options
        std::function<void(const char *)> progressCallback = nullptr;

        if (options)
        {
            // For now, we'll use a simple approach since OSG progress callback API varies by version
            // Just enable progress logging if options are provided
            PluginLogger::logDebug("LMB", "OSG options provided, enabling progress logging");
            progressCallback = [](const char *msg)
            {
                PluginLogger::logDebug("LMB", std::string("Progress: ") + msg);
            };

            // Parse additional options from option string
            std::string optionString = options->getOptionString();
            if (!optionString.empty())
            {
                PluginLogger::logDebug("LMB", "Processing options: " + optionString);

                // Check for debug option
                if (optionString.find("debug") != std::string::npos)
                {
                    PluginLogger::logDebug("LMB", "Debug mode enabled via options");
                    PluginLogger::setLogLevel(PluginLogger::LOG_DEBUG);
                }

                // Check for verbose option
                if (optionString.find("verbose") != std::string::npos)
                {
                    PluginLogger::logInfo("LMB", "Verbose mode enabled via options");
                    if (!progressCallback)
                    {
                        progressCallback = [](const char *msg)
                        {
                            PluginLogger::logInfo("LMB", std::string("Progress: ") + msg);
                        };
                    }
                }

                // Check for unsupported options and warn
                std::vector<std::string> knownOptions = {"debug", "verbose"};
                std::istringstream iss(optionString);
                std::string option;
                while (iss >> option)
                {
                    bool isKnown = false;
                    for (const auto &known : knownOptions)
                    {
                        if (option.find(known) != std::string::npos)
                        {
                            isKnown = true;
                            break;
                        }
                    }
                    if (!isKnown && !option.empty())
                    {
                        PluginLogger::logWarning("LMB", "Ignoring unsupported option: " + option);
                    }
                }
            }
        }

        // 使用独立的 LmbParser 加载文件，传递进度回调
        osg::ref_ptr<osg::Group> result = LmbPlugin::LmbParser::parseFile(localFileName, progressCallback);

        if (result.valid())
        {
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            PluginLogger::logInfo("LMB", "Successfully loaded " + fileName + " in " +
                                             std::to_string(duration.count()) + "ms");
            return ReadResult(result.release());
        }
        else
        {
            PluginLogger::logError("LMB", "Failed to load " + fileName + " - parser returned null");
            return ReadResult::ERROR_IN_READING_FILE;
        }
    }
    catch (const LmbPlugin::LmbParseException &e)
    {
        const LmbPlugin::LmbError &error = e.getError();
        PluginLogger::logError("LMB", "Parse exception while loading " + fileName + ": " +
                                          error.message + " (type: " + std::to_string(static_cast<int>(error.type)) + ")");
        return ReadResult::ERROR_IN_READING_FILE;
    }
    catch (const std::exception &e)
    {
        PluginLogger::logError("LMB", "Standard exception while loading " + fileName + ": " + e.what());
        return ReadResult::ERROR_IN_READING_FILE;
    }
    catch (...)
    {
        PluginLogger::logError("LMB", "Unknown exception while loading " + fileName);
        return ReadResult::ERROR_IN_READING_FILE;
    }
}

osgDB::ReaderWriter::ReadResult ReaderWriterLMB::readNode(std::istream &stream, const osgDB::Options *options) const
{
    // LMB 格式需要随机访问，不支持流读取
    return ReadResult::FILE_NOT_HANDLED;
}

osgDB::ReaderWriter::WriteResult ReaderWriterLMB::writeNode(const osg::Node &node, const std::string &fileName, const osgDB::Options *options) const
{
    std::string ext = osgDB::getLowerCaseFileExtension(fileName);
    if (!acceptsExtension(ext))
    {
        PluginLogger::logDebug("LMB", "File extension not supported for writing: " + ext);
        return WriteResult::FILE_NOT_HANDLED;
    }

    // LMB 格式目前不支持写入
    PluginLogger::logWarning("LMB", "Writing LMB files is not currently supported");
    return WriteResult::FILE_NOT_HANDLED;
}

// 注册插件
REGISTER_OSGPLUGIN(lmb, ReaderWriterLMB)