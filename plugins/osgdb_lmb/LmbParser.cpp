#include "LmbParser.h"
#include <osg/LightModel>
#include <osg/CullFace>
#include <osg/Depth>
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <osg/GL>
#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/MatrixTransform>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <filesystem>

namespace LmbPlugin
{

    // OSG类型定义
    using Vec3Array = osg::Vec3Array;
    using DrawElementsUInt = osg::DrawElementsUInt;

    osg::ref_ptr<osg::Group> LmbParser::parseFile(const std::string &filepath)
    {
        return parseFile(filepath, nullptr);
    }

    osg::ref_ptr<osg::Group> LmbParser::parseFile(const std::string &filepath,
                                                  std::function<void(const char *)> progressCb)
    {
        auto startTime = std::chrono::high_resolution_clock::now();

        try
        {
            // Validate file access first
            validateFileAccess(filepath);

            Vector3f scenePosition;
            std::vector<uint32_t> colors;
            std::vector<Node> nodes;

            PluginLogger::logFileLoadStart("LMB", filepath);
            if (progressCb)
                progressCb("开始读取 LMB 文件...");

            if (!ReadFile(filepath, scenePosition, colors, nodes, progressCb))
            {
                logError(LmbErrorType::CORRUPTED_DATA, "Failed to read LMB file data", filepath);
                return nullptr;
            }

            // Validate loaded data
            validateHeader(scenePosition, colors.size(), nodes.size());
            validateColorData(colors, colors.size());

            for (size_t i = 0; i < nodes.size(); ++i)
            {
                validateNodeData(nodes[i], colors.size());
            }

            // 创建根节点
            osg::ref_ptr<osg::Group> root = new osg::Group;
            root->setName("meshRoot");

            // 创建场景变换节点
            osg::ref_ptr<osg::MatrixTransform> sceneTransform = new osg::MatrixTransform;
            sceneTransform->setName("SceneTransform");
            sceneTransform->setMatrix(osg::Matrix::translate(
                scenePosition.x, scenePosition.y, scenePosition.z));
            root->addChild(sceneTransform);

            // 设置场景状态
            SetupSceneState(root);

            // 处理每个节点
            const size_t totalNodes = nodes.size();
            size_t nextReportBuild = 0;
            for (size_t nodeIndex = 0; nodeIndex < totalNodes; ++nodeIndex)
            {
                const auto &node = nodes[nodeIndex];

                // 设置节点名称
                std::string nodeName = node.name.empty() ? ("Node_" + std::to_string(nodeIndex)) : node.name;
                if (!node.instances.empty())
                {
                    // 每个实例独立节点（不合并）
                    // 先创建基础几何与原始节点的共享状态
                    osg::ref_ptr<osg::Geometry> baseGeom = CreateGeometry(node);
                    // 为主节点（自身）创建一个实例
                    {
                        osg::ref_ptr<osg::MatrixTransform> nodeTransform = new osg::MatrixTransform;
                        nodeTransform->setName(nodeName);
                        nodeTransform->setMatrix(CreateTransformMatrix(node.matrix, node.position));

                        osg::ref_ptr<osg::StateSet> state = CreateSharedState(colors[node.colorIndex]);
                        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
                        geode->setName(nodeName + "_Geode");
                        geode->addDrawable(baseGeom.get());
                        geode->setStateSet(state.get());
                        nodeTransform->addChild(geode.get());
                        sceneTransform->addChild(nodeTransform.get());
                    }
                    // 其他实例
                    for (size_t i = 0; i < node.instances.size(); ++i)
                    {
                        const auto &inst = node.instances[i];
                        osg::ref_ptr<osg::MatrixTransform> instTransform = new osg::MatrixTransform;
                        std::string instName = nodeName + std::string("_inst_") + std::to_string(i);
                        instTransform->setName(instName);
                        instTransform->setMatrix(CreateTransformMatrix(inst.matrix, inst.position));

                        osg::ref_ptr<osg::StateSet> state = CreateSharedState(colors[inst.colorIndex]);
                        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
                        geode->setName(instName + "_Geode");
                        geode->addDrawable(baseGeom.get());
                        geode->setStateSet(state.get());
                        instTransform->addChild(geode.get());
                        sceneTransform->addChild(instTransform.get());
                    }
                }
                else
                {
                    // 处理非实例化节点
                    osg::ref_ptr<osg::MatrixTransform> nodeTransform = new osg::MatrixTransform;
                    nodeTransform->setName(nodeName);
                    nodeTransform->setMatrix(CreateTransformMatrix(node.matrix, node.position));

                    osg::ref_ptr<osg::Geometry> geometry = CreateGeometry(node);
                    osg::ref_ptr<osg::StateSet> state = CreateSharedState(colors[node.colorIndex]);

                    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
                    geode->setName(nodeName + "_Geode");
                    geode->addDrawable(geometry);
                    geode->setStateSet(state);

                    nodeTransform->addChild(geode);
                    sceneTransform->addChild(nodeTransform);
                }

                // 构建进度（不频繁）：每处理约2%节点汇报一次
                if (progressCb && nodeIndex >= nextReportBuild)
                {
                    int percent = 10 + int((double(nodeIndex + 1) / double(totalNodes)) * 90.0); // 读取占10%，构建占90%
                    std::string msg = std::string("构建场景 ") + std::to_string(nodeIndex + 1) + "/" + std::to_string(totalNodes) +
                                      " (" + std::to_string(percent) + "%)";
                    progressCb(msg.c_str());
                    nextReportBuild = nodeIndex + std::max<size_t>(1, totalNodes / 50);
                }
            }

            // Log successful loading
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            PluginLogger::logFileLoadSuccess("LMB", filepath, duration.count(), static_cast<int>(nodes.size()));

            return root;
        }
        catch (const LmbParseException &e)
        {
            const LmbError &error = e.getError();
            PluginLogger::logFileLoadFailure("LMB", filepath,
                                             getErrorTypeString(error.type) + ": " + error.message);
            return nullptr;
        }
        catch (const std::exception &e)
        {
            logError(LmbErrorType::UNKNOWN_ERROR, std::string("Standard exception: ") + e.what(), filepath);
            return nullptr;
        }
        catch (...)
        {
            logError(LmbErrorType::UNKNOWN_ERROR, "Unknown exception occurred", filepath);
            return nullptr;
        }
    }

    void LmbParser::SetupSceneState(osg::ref_ptr<osg::Group> root)
    {
        osg::ref_ptr<osg::StateSet> stateSet = root->getOrCreateStateSet();

        // 设置环境光
        osg::ref_ptr<osg::LightModel> lightModel = new osg::LightModel;
        lightModel->setAmbientIntensity(osg::Vec4(0.3f, 0.3f, 0.3f, 1.0f));
        lightModel->setTwoSided(true);
        stateSet->setAttribute(lightModel);

        // 设置深度测试
        osg::ref_ptr<osg::Depth> depth = new osg::Depth;
        depth->setWriteMask(true);
        depth->setFunction(osg::Depth::LESS);
        stateSet->setAttribute(depth);

        // 禁用面剔除
        stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);

        // 启用深度测试
        stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    }

    osg::ref_ptr<osg::StateSet> LmbParser::CreateSharedState(uint32_t color)
    {
        osg::ref_ptr<osg::StateSet> stateSet = new osg::StateSet;
        osg::Vec4 colorVec = CreateColorFromRGB(color);
        stateSet->setAttribute(CreateMaterial(colorVec));
        stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
        stateSet->setMode(GL_BLEND, osg::StateAttribute::OFF);
        stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
        stateSet->setRenderingHint(osg::StateSet::OPAQUE_BIN);
        return stateSet;
    }

    osg::ref_ptr<osg::Material> LmbParser::CreateMaterial(const osg::Vec4 &color)
    {
        osg::ref_ptr<osg::Material> material = new osg::Material;
        material->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
        material->setAmbient(osg::Material::FRONT_AND_BACK, color * 0.6f);
        material->setDiffuse(osg::Material::FRONT_AND_BACK, color * 0.8f);
        material->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(0.2f, 0.2f, 0.2f, 1.0f));
        material->setAlpha(osg::Material::FRONT_AND_BACK, 1.0f);
        return material;
    }

    bool LmbParser::ReadFile(const std::string &filepath, Vector3f &scenePosition,
                             std::vector<uint32_t> &colors, std::vector<Node> &nodes,
                             std::function<void(const char *)> progressCb)
    {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open())
        {
            logError(LmbErrorType::FILE_ACCESS_ERROR, "Cannot open file for reading", filepath);
            return false;
        }

        try
        {
            // Read header
            uint32_t colorCount, nodeCount;
            if (!ReadHeader(file, scenePosition, colorCount, nodeCount))
            {
                logError(LmbErrorType::CORRUPTED_HEADER, "Failed to read file header", filepath, file.tellg());
                return false;
            }
            validateStreamState(file, "header reading", file.tellg());
            if (progressCb)
                progressCb("已读取文件头...");

            // Read colors
            colors.resize(colorCount);
            if (!ReadColors(file, colorCount, colors))
            {
                logError(LmbErrorType::CORRUPTED_DATA, "Failed to read color data", filepath, file.tellg());
                return false;
            }
            validateStreamState(file, "color reading", file.tellg());
            if (progressCb)
                progressCb("已读取颜色表...");

            // Read nodes
            nodes.resize(nodeCount);
            uint32_t nextReport = 0;
            for (uint32_t i = 0; i < nodeCount; ++i)
            {
                long nodeStartPos = file.tellg();
                if (!ReadNode(file, nodes[i]))
                {
                    std::ostringstream oss;
                    oss << "Failed to read node " << i << " of " << nodeCount;
                    logError(LmbErrorType::CORRUPTED_DATA, oss.str(), filepath, nodeStartPos);
                    return false;
                }
                validateStreamState(file, "node " + std::to_string(i) + " reading", nodeStartPos);

                if (progressCb && i >= nextReport)
                {
                    int pct = int((double(i + 1) / double(nodeCount)) * 10.0); // 读取阶段最多10%
                    std::string msg = std::string("读取节点 ") + std::to_string(i + 1) + "/" + std::to_string(nodeCount) +
                                      " (" + std::to_string(pct) + "%)";
                    progressCb(msg.c_str());
                    nextReport = i + std::max<uint32_t>(1, nodeCount / 50);
                }
            }

            return true;
        }
        catch (const LmbParseException &e)
        {
            // Re-throw to be caught by the main parseFile method
            throw e;
        }
        catch (const std::exception &e)
        {
            logError(LmbErrorType::UNKNOWN_ERROR, std::string("Exception during file reading: ") + e.what(), filepath);
            return false;
        }
    }

    bool LmbParser::ReadColors(std::istream &stream,
                               uint32_t colorCount,
                               std::vector<uint32_t> &colors)
    {
        stream.read(reinterpret_cast<char *>(colors.data()),
                    colorCount * sizeof(uint32_t));
        return stream.good();
    }

    bool LmbParser::ReadInstances(std::istream &stream, std::vector<Instance> &instances)
    {
        uint32_t instanceCount;
        stream.read(reinterpret_cast<char *>(&instanceCount), sizeof(uint32_t));

        instances.resize(instanceCount);
        for (uint32_t i = 0; i < instanceCount; ++i)
        {
            Instance &instance = instances[i];

            // Read name
            uint16_t nameLength;
            stream.read(reinterpret_cast<char *>(&nameLength), sizeof(uint16_t));
            std::vector<char> nameBuffer(nameLength);
            stream.read(nameBuffer.data(), nameLength);
            instance.name = std::string(nameBuffer.data(), nameLength);
            AlignTo4Bytes(stream);

            // 读取3x3变换矩阵
            stream.read(reinterpret_cast<char *>(&instance.matrix), sizeof(float) * 9);

            // 读取位置
            stream.read(reinterpret_cast<char *>(&instance.position), sizeof(Vector3f));

            // Read color index
            stream.read(reinterpret_cast<char *>(&instance.colorIndex), sizeof(uint32_t));
        }

        return stream.good();
    }

    bool LmbParser::ReadHeader(std::istream &stream,
                               Vector3f &position,
                               uint32_t &colorCount,
                               uint32_t &nodeCount)
    {
        stream.read(reinterpret_cast<char *>(&position), sizeof(Vector3f));
        stream.read(reinterpret_cast<char *>(&colorCount), sizeof(uint32_t));
        stream.read(reinterpret_cast<char *>(&nodeCount), sizeof(uint32_t));
        return stream.good();
    }

    bool LmbParser::ReadNode(std::istream &stream, Node &node)
    {
        // Read name
        uint16_t nameLength;
        stream.read(reinterpret_cast<char *>(&nameLength), sizeof(uint16_t));
        std::vector<char> nameBuffer(nameLength);
        stream.read(nameBuffer.data(), nameLength);
        node.name = std::string(nameBuffer.data(), nameLength);
        AlignTo4Bytes(stream);

        // 读取3x3变换矩阵
        stream.read(reinterpret_cast<char *>(&node.matrix), sizeof(float) * 9);

        // 读取位置
        stream.read(reinterpret_cast<char *>(&node.position), sizeof(Vector3f));

        // 读取压缩顶点数据
        // baseVertex (3 floats)
        stream.read(reinterpret_cast<char *>(&node.baseVertex), sizeof(Vector3f));

        // vertexScale (3 floats)
        stream.read(reinterpret_cast<char *>(&node.vertexScale), sizeof(Vector3f));

        // 顶点数量
        uint32_t vertexCount;
        stream.read(reinterpret_cast<char *>(&vertexCount), sizeof(uint32_t));

        // 压缩顶点数据 (short数组，长度为(vertexCount-1)*3)
        uint32_t compressedVertexCount = (vertexCount - 1) * 3;
        node.compressVertices.resize(compressedVertexCount);
        stream.read(reinterpret_cast<char *>(node.compressVertices.data()),
                    compressedVertexCount * sizeof(int16_t));
        AlignTo4Bytes(stream);

        // 读取压缩法线数据
        uint32_t normalCount = vertexCount;
        node.normals.resize(normalCount);
        stream.read(reinterpret_cast<char *>(node.normals.data()),
                    normalCount * sizeof(int32_t));
        AlignTo4Bytes(stream);

        // Read indices
        uint32_t indexCount;
        stream.read(reinterpret_cast<char *>(&indexCount), sizeof(uint32_t));
        node.indices.resize(indexCount);

        if (vertexCount <= 255)
        {
            std::vector<uint8_t> tempIndices(indexCount);
            stream.read(reinterpret_cast<char *>(tempIndices.data()), indexCount);
            std::transform(tempIndices.begin(), tempIndices.end(),
                           node.indices.begin(),
                           [](uint8_t v)
                           { return static_cast<uint32_t>(v); });
            AlignTo4Bytes(stream);
        }
        else if (vertexCount <= 65535)
        {
            std::vector<uint16_t> tempIndices(indexCount);
            stream.read(reinterpret_cast<char *>(tempIndices.data()),
                        indexCount * sizeof(uint16_t));
            std::transform(tempIndices.begin(), tempIndices.end(),
                           node.indices.begin(),
                           [](uint16_t v)
                           { return static_cast<uint32_t>(v); });
            AlignTo4Bytes(stream);
        }
        else
        {
            stream.read(reinterpret_cast<char *>(node.indices.data()),
                        indexCount * sizeof(uint32_t));
        }

        // Read color index
        stream.read(reinterpret_cast<char *>(&node.colorIndex), sizeof(uint32_t));

        // Read instances
        return ReadInstances(stream, node.instances);
    }

    void LmbParser::AlignTo4Bytes(std::istream &stream)
    {
        auto pos = stream.tellg();
        auto padding = (4 - (pos % 4)) % 4;
        if (padding > 0)
        {
            stream.seekg(padding, std::ios::cur);
        }
    }

    osg::ref_ptr<osg::Geometry> LmbParser::CreateGeometry(const Node &node)
    {
        osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;

        // 解压顶点数据
        osg::ref_ptr<osg::Vec3Array> vertices = DecompressVertices(node);
        geometry->setVertexArray(vertices);

        // 法线
        osg::ref_ptr<osg::Vec3Array> normals = DecodeNormals(node.normals);
        geometry->setNormalArray(normals, osg::Array::BIND_PER_VERTEX);

        // 索引
        osg::ref_ptr<osg::DrawElementsUInt> indices =
            new osg::DrawElementsUInt(GL_TRIANGLES);
        indices->insert(indices->end(),
                        node.indices.begin(),
                        node.indices.end());
        geometry->addPrimitiveSet(indices);

        // 启用显示列表以提高渲染性能
        geometry->setUseDisplayList(true);
        geometry->setUseVertexBufferObjects(true);

        return geometry;
    }

    osg::ref_ptr<osg::Vec3Array> LmbParser::DecodeNormals(const std::vector<int32_t> &encodedNormals)
    {
        osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;

        for (size_t i = 0; i < encodedNormals.size(); i += 1)
        {
            uint32_t packed = static_cast<uint32_t>(encodedNormals[i]);

            // 提取各个分量 (每个分量使用10位)
            int x = (packed >> 20) & 0x3FF; // 取高10位
            int y = (packed >> 10) & 0x3FF; // 取中间10位
            int z = packed & 0x3FF;         // 取低10位

            // 处理负数情况 (从0~1023转换回-512~511范围)
            x = x >= 512 ? x - 1024 : x;
            y = y >= 512 ? y - 1024 : y;
            z = z >= 512 ? z - 1024 : z;

            // 将整数值转换为浮点数并进行缩放
            float nx = x / 511.0f;
            float ny = y / 511.0f;
            float nz = z / 511.0f;

            osg::Vec3 normal(nx, ny, nz);
            normal.normalize();
            normals->push_back(normal);
        }

        return normals;
    }

    osg::Matrix LmbParser::CreateTransformMatrix(const float matrix[9], const Vector3f &position)
    {
        osg::Matrix transform;

        // 设置3x3变换部分
        transform(0, 0) = matrix[0];
        transform(0, 1) = matrix[1];
        transform(0, 2) = matrix[2];
        transform(1, 0) = matrix[3];
        transform(1, 1) = matrix[4];
        transform(1, 2) = matrix[5];
        transform(2, 0) = matrix[6];
        transform(2, 1) = matrix[7];
        transform(2, 2) = matrix[8];

        // 设置位置（平移）部分
        transform(3, 0) = position.x;
        transform(3, 1) = position.y;
        transform(3, 2) = position.z;

        // 设置齐次坐标部分
        transform(0, 3) = 0.0f;
        transform(1, 3) = 0.0f;
        transform(2, 3) = 0.0f;
        transform(3, 3) = 1.0f;

        return transform;
    }

    osg::ref_ptr<osg::Vec3Array> LmbParser::DecompressVertices(const Node &node)
    {
        // 按 OCC 规范：写入 q = (value - base) * scale；读取 value = base + q / scale
        osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
        // 基准点
        vertices->push_back(osg::Vec3(node.baseVertex.x, node.baseVertex.y, node.baseVertex.z));
        // 保护 scale 分量，避免除以 0
        const float sx = (std::abs(node.vertexScale.x) > 1e-12f) ? node.vertexScale.x : 1.0f;
        const float sy = (std::abs(node.vertexScale.y) > 1e-12f) ? node.vertexScale.y : 1.0f;
        const float sz = (std::abs(node.vertexScale.z) > 1e-12f) ? node.vertexScale.z : 1.0f;
        for (size_t i = 0; i < node.compressVertices.size(); i += 3)
        {
            float qx = static_cast<float>(node.compressVertices[i]);
            float qy = static_cast<float>(node.compressVertices[i + 1]);
            float qz = static_cast<float>(node.compressVertices[i + 2]);
            float x = node.baseVertex.x + qx / sx;
            float y = node.baseVertex.y + qy / sy;
            float z = node.baseVertex.z + qz / sz;
            vertices->push_back(osg::Vec3(x, y, z));
        }
        return vertices;
    }

    osg::Vec4 LmbParser::CreateColorFromRGB(uint32_t color)
    {
        return osg::Vec4(
            ((color >> 16) & 0xFF) / 255.0f, // R
            ((color >> 8) & 0xFF) / 255.0f,  // G
            (color & 0xFF) / 255.0f,         // B
            1.0f                             // A
        );
    }

    // Error handling methods implementation
    void LmbParser::validateFileAccess(const std::string &filepath)
    {
        if (filepath.empty())
        {
            throw LmbParseException(LmbError(LmbErrorType::FILE_NOT_FOUND, "File path is empty", filepath));
        }

        std::filesystem::path path(filepath);
        if (!std::filesystem::exists(path))
        {
            throw LmbParseException(LmbError(LmbErrorType::FILE_NOT_FOUND, "File does not exist", filepath));
        }

        if (!std::filesystem::is_regular_file(path))
        {
            throw LmbParseException(LmbError(LmbErrorType::FILE_ACCESS_ERROR, "Path is not a regular file", filepath));
        }

        // Check file size
        auto fileSize = std::filesystem::file_size(path);
        if (fileSize == 0)
        {
            throw LmbParseException(LmbError(LmbErrorType::INVALID_FORMAT, "File is empty", filepath));
        }

        // Check minimum file size (header should be at least 20 bytes)
        if (fileSize < 20)
        {
            throw LmbParseException(LmbError(LmbErrorType::INVALID_FORMAT, "File too small to be valid LMB format", filepath));
        }
    }

    void LmbParser::validateHeader(const Vector3f &position, uint32_t colorCount, uint32_t nodeCount)
    {
        // Validate position values
        if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z))
        {
            throw LmbParseException(LmbError(LmbErrorType::CORRUPTED_HEADER, "Invalid scene position values"));
        }

        // Validate color count
        if (colorCount == 0)
        {
            throw LmbParseException(LmbError(LmbErrorType::INVALID_COLOR_COUNT, "Color count cannot be zero"));
        }

        if (colorCount > 1000000)
        { // Reasonable upper limit
            throw LmbParseException(LmbError(LmbErrorType::INVALID_COLOR_COUNT, "Color count exceeds reasonable limit"));
        }

        // Validate node count
        if (nodeCount == 0)
        {
            throw LmbParseException(LmbError(LmbErrorType::INVALID_NODE_COUNT, "Node count cannot be zero"));
        }

        if (nodeCount > 1000000)
        { // Reasonable upper limit
            throw LmbParseException(LmbError(LmbErrorType::INVALID_NODE_COUNT, "Node count exceeds reasonable limit"));
        }
    }

    void LmbParser::validateColorData(const std::vector<uint32_t> &colors, uint32_t expectedCount)
    {
        if (colors.size() != expectedCount)
        {
            std::ostringstream oss;
            oss << "Color data size mismatch: expected " << expectedCount << ", got " << colors.size();
            throw LmbParseException(LmbError(LmbErrorType::CORRUPTED_DATA, oss.str()));
        }

        // Validate color values (basic sanity check)
        for (size_t i = 0; i < colors.size(); ++i)
        {
            uint32_t color = colors[i];
            // Check if color has valid RGB components (allow any value, but log suspicious ones)
            if (color == 0x00000000 || color == 0xFFFFFFFF)
            {
                PluginLogger::logWarning("LMB", "Suspicious color value at index " + std::to_string(i) +
                                                    ": 0x" + std::to_string(color));
            }
        }
    }

    void LmbParser::validateNodeData(const Node &node, uint32_t colorCount)
    {
        // Validate node name (allow empty names)
        if (node.name.length() > 1000)
        { // Reasonable limit
            throw LmbParseException(LmbError(LmbErrorType::CORRUPTED_DATA, "Node name too long: " + node.name));
        }

        // Validate transformation matrix
        for (int i = 0; i < 9; ++i)
        {
            if (!std::isfinite(node.matrix[i]))
            {
                throw LmbParseException(LmbError(LmbErrorType::CORRUPTED_DATA, "Invalid transformation matrix in node: " + node.name));
            }
        }

        // Validate position
        if (!std::isfinite(node.position.x) || !std::isfinite(node.position.y) || !std::isfinite(node.position.z))
        {
            throw LmbParseException(LmbError(LmbErrorType::CORRUPTED_DATA, "Invalid position in node: " + node.name));
        }

        // Validate base vertex and scale
        if (!std::isfinite(node.baseVertex.x) || !std::isfinite(node.baseVertex.y) || !std::isfinite(node.baseVertex.z))
        {
            throw LmbParseException(LmbError(LmbErrorType::INVALID_VERTEX_DATA, "Invalid base vertex in node: " + node.name));
        }

        if (!std::isfinite(node.vertexScale.x) || !std::isfinite(node.vertexScale.y) || !std::isfinite(node.vertexScale.z))
        {
            throw LmbParseException(LmbError(LmbErrorType::INVALID_VERTEX_DATA, "Invalid vertex scale in node: " + node.name));
        }

        // Validate vertex data consistency
        if (node.compressVertices.size() % 3 != 0)
        {
            throw LmbParseException(LmbError(LmbErrorType::INVALID_VERTEX_DATA, "Compressed vertex data size not divisible by 3 in node: " + node.name));
        }

        size_t expectedVertexCount = (node.compressVertices.size() / 3) + 1; // +1 for base vertex
        if (node.normals.size() != expectedVertexCount)
        {
            std::ostringstream oss;
            oss << "Normal count mismatch in node " << node.name << ": expected " << expectedVertexCount << ", got " << node.normals.size();
            throw LmbParseException(LmbError(LmbErrorType::INVALID_VERTEX_DATA, oss.str()));
        }

        // Validate indices
        if (node.indices.empty())
        {
            throw LmbParseException(LmbError(LmbErrorType::INVALID_INDEX_DATA, "No indices in node: " + node.name));
        }

        if (node.indices.size() % 3 != 0)
        {
            throw LmbParseException(LmbError(LmbErrorType::INVALID_INDEX_DATA, "Index count not divisible by 3 in node: " + node.name));
        }

        // Validate index values
        for (size_t i = 0; i < node.indices.size(); ++i)
        {
            if (node.indices[i] >= expectedVertexCount)
            {
                std::ostringstream oss;
                oss << "Index out of range in node " << node.name << ": index " << node.indices[i] << " >= vertex count " << expectedVertexCount;
                throw LmbParseException(LmbError(LmbErrorType::INVALID_INDEX_DATA, oss.str()));
            }
        }

        // Validate color index
        if (node.colorIndex >= colorCount)
        {
            std::ostringstream oss;
            oss << "Color index out of range in node " << node.name << ": " << node.colorIndex << " >= " << colorCount;
            throw LmbParseException(LmbError(LmbErrorType::CORRUPTED_DATA, oss.str()));
        }

        // Validate instances
        for (size_t i = 0; i < node.instances.size(); ++i)
        {
            const Instance &inst = node.instances[i];

            // Validate instance transformation matrix
            for (int j = 0; j < 9; ++j)
            {
                if (!std::isfinite(inst.matrix[j]))
                {
                    std::ostringstream oss;
                    oss << "Invalid transformation matrix in instance " << i << " of node: " << node.name;
                    throw LmbParseException(LmbError(LmbErrorType::CORRUPTED_DATA, oss.str()));
                }
            }

            // Validate instance position
            if (!std::isfinite(inst.position.x) || !std::isfinite(inst.position.y) || !std::isfinite(inst.position.z))
            {
                std::ostringstream oss;
                oss << "Invalid position in instance " << i << " of node: " << node.name;
                throw LmbParseException(LmbError(LmbErrorType::CORRUPTED_DATA, oss.str()));
            }

            // Validate instance color index
            if (inst.colorIndex >= colorCount)
            {
                std::ostringstream oss;
                oss << "Color index out of range in instance " << i << " of node " << node.name << ": " << inst.colorIndex << " >= " << colorCount;
                throw LmbParseException(LmbError(LmbErrorType::CORRUPTED_DATA, oss.str()));
            }
        }
    }

    void LmbParser::validateStreamState(std::istream &stream, const std::string &operation, long position)
    {
        if (stream.fail())
        {
            std::ostringstream oss;
            oss << "Stream error during " << operation;
            if (position >= 0)
            {
                oss << " at position " << position;
            }
            throw LmbParseException(LmbError(LmbErrorType::CORRUPTED_DATA, oss.str(), "", position));
        }

        if (stream.eof())
        {
            std::ostringstream oss;
            oss << "Unexpected end of file during " << operation;
            if (position >= 0)
            {
                oss << " at position " << position;
            }
            throw LmbParseException(LmbError(LmbErrorType::CORRUPTED_DATA, oss.str(), "", position));
        }
    }

    std::string LmbParser::getErrorTypeString(LmbErrorType type)
    {
        switch (type)
        {
        case LmbErrorType::FILE_NOT_FOUND:
            return "FILE_NOT_FOUND";
        case LmbErrorType::FILE_ACCESS_ERROR:
            return "FILE_ACCESS_ERROR";
        case LmbErrorType::INVALID_FORMAT:
            return "INVALID_FORMAT";
        case LmbErrorType::CORRUPTED_HEADER:
            return "CORRUPTED_HEADER";
        case LmbErrorType::CORRUPTED_DATA:
            return "CORRUPTED_DATA";
        case LmbErrorType::MEMORY_ERROR:
            return "MEMORY_ERROR";
        case LmbErrorType::INVALID_COLOR_COUNT:
            return "INVALID_COLOR_COUNT";
        case LmbErrorType::INVALID_NODE_COUNT:
            return "INVALID_NODE_COUNT";
        case LmbErrorType::INVALID_VERTEX_DATA:
            return "INVALID_VERTEX_DATA";
        case LmbErrorType::INVALID_INDEX_DATA:
            return "INVALID_INDEX_DATA";
        case LmbErrorType::UNKNOWN_ERROR:
            return "UNKNOWN_ERROR";
        default:
            return "UNKNOWN_ERROR_TYPE";
        }
    }

    void LmbParser::logError(LmbErrorType type, const std::string &message, const std::string &fileName, long position)
    {
        std::ostringstream oss;
        oss << getErrorTypeString(type) << ": " << message;
        if (!fileName.empty())
        {
            oss << " (file: " << fileName << ")";
        }
        if (position >= 0)
        {
            oss << " (position: " << position << ")";
        }
        PluginLogger::logError("LMB", oss.str());
    }

} // namespace LmbPlugin