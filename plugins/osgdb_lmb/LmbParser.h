#ifndef LMBPARSER_H
#define LMBPARSER_H

#include <osg/Group>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/StateSet>
#include <osg/Material>
#include <osg/Array>
#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/Matrix>
#include <osg/Geode>
#include <string>
#include <vector>
#include <iostream>
#include <functional>
#include <stdexcept>
#include "../PluginLogger.h"

namespace LmbPlugin
{

    /**
     * @brief Error types for LMB parsing
     */
    enum class LmbErrorType
    {
        FILE_NOT_FOUND,
        FILE_ACCESS_ERROR,
        INVALID_FORMAT,
        CORRUPTED_HEADER,
        CORRUPTED_DATA,
        MEMORY_ERROR,
        INVALID_COLOR_COUNT,
        INVALID_NODE_COUNT,
        INVALID_VERTEX_DATA,
        INVALID_INDEX_DATA,
        UNKNOWN_ERROR
    };

    /**
     * @brief Error information structure
     */
    struct LmbError
    {
        LmbErrorType type;
        std::string message;
        std::string fileName;
        long filePosition;

        LmbError(LmbErrorType t, const std::string &msg, const std::string &file = "", long pos = -1)
            : type(t), message(msg), fileName(file), filePosition(pos) {}
    };

    /**
     * @brief Exception class for LMB parsing errors
     */
    class LmbParseException : public std::runtime_error
    {
    public:
        LmbParseException(const LmbError &error)
            : std::runtime_error(error.message), error_(error) {}

        const LmbError &getError() const { return error_; }

    private:
        LmbError error_;
    };

    struct Vector3f
    {
        float x, y, z;
    };

    struct Instance
    {
        std::string name;
        float matrix[9]; // 3x3变换矩阵
        Vector3f position;
        uint32_t colorIndex;
    };

    struct Node
    {
        std::string name;
        float matrix[9]; // 3x3变换矩阵
        Vector3f position;

        // 压缩顶点数据
        Vector3f baseVertex;
        Vector3f vertexScale;
        std::vector<int16_t> compressVertices;

        std::vector<int32_t> normals; // 压缩法线
        std::vector<uint32_t> indices;
        uint32_t colorIndex;
        std::vector<Instance> instances;
    };

    class LmbParser
    {
    public:
        static osg::ref_ptr<osg::Group> parseFile(const std::string &filepath);
        // 支持进度回调的重载（文本提示），回调不要太频繁
        static osg::ref_ptr<osg::Group> parseFile(const std::string &filepath,
                                                  std::function<void(const char *)> progressCb);

    private:
        // Error handling methods
        static void validateFileAccess(const std::string &filepath);
        static void validateHeader(const Vector3f &position, uint32_t colorCount, uint32_t nodeCount);
        static void validateColorData(const std::vector<uint32_t> &colors, uint32_t expectedCount);
        static void validateNodeData(const Node &node, uint32_t colorCount);
        static void validateStreamState(std::istream &stream, const std::string &operation, long position = -1);
        static std::string getErrorTypeString(LmbErrorType type);
        static void logError(LmbErrorType type, const std::string &message, const std::string &fileName = "", long position = -1);
        static void SetupSceneState(osg::ref_ptr<osg::Group> root);
        static osg::ref_ptr<osg::StateSet> CreateSharedState(uint32_t color);
        static osg::ref_ptr<osg::Material> CreateMaterial(const osg::Vec4 &color);
        static bool ReadFile(const std::string &filepath, Vector3f &scenePosition,
                             std::vector<uint32_t> &colors, std::vector<Node> &nodes,
                             std::function<void(const char *)> progressCb = nullptr);

        static bool ReadColors(std::istream &stream, uint32_t colorCount, std::vector<uint32_t> &colors);
        static bool ReadInstances(std::istream &stream, std::vector<Instance> &instances);
        static bool ReadHeader(std::istream &stream, Vector3f &position, uint32_t &colorCount, uint32_t &nodeCount);
        static bool ReadNode(std::istream &stream, Node &node);
        static void AlignTo4Bytes(std::istream &stream);
        static osg::ref_ptr<osg::Geometry> CreateGeometry(const Node &node);
        static osg::ref_ptr<osg::Vec3Array> DecodeNormals(const std::vector<int32_t> &encodedNormals);
        static osg::ref_ptr<osg::Vec3Array> DecompressVertices(const Node &node);
        static osg::Matrix CreateTransformMatrix(const float matrix[9], const Vector3f &position);
        static osg::Vec4 CreateColorFromRGB(uint32_t color);
    };

} // namespace LmbPlugin

#endif // LMBPARSER_H