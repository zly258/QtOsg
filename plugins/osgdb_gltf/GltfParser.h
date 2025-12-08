#ifndef GLTFPARSER_H
#define GLTFPARSER_H

#include <tiny_gltf.h>
#include <osg/Group>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Material>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osg/StateSet>
#include <osg/BlendFunc>
#include <osg/AlphaFunc>
#include <osg/CullFace>
#include <osg/Image>
#include <string>
#include <functional>
#include <map>
#include <vector>
#include <stdexcept>
#include "../PluginLogger.h"

/**
 * @brief Error types for GLTF parsing
 */
enum class GltfErrorType
{
    FILE_NOT_FOUND,
    FILE_ACCESS_ERROR,
    INVALID_FORMAT,
    UNSUPPORTED_FORMAT,
    CORRUPTED_DATA,
    INVALID_SCENE_INDEX,
    INVALID_NODE_INDEX,
    INVALID_MESH_INDEX,
    INVALID_MATERIAL_INDEX,
    INVALID_TEXTURE_INDEX,
    INVALID_IMAGE_INDEX,
    INVALID_ACCESSOR_INDEX,
    INVALID_BUFFER_VIEW_INDEX,
    MEMORY_ERROR,
    TINYGLTF_ERROR,
    UNKNOWN_ERROR
};

/**
 * @brief Error information structure for GLTF parsing
 */
struct GltfError
{
    GltfErrorType type;
    std::string message;
    std::string fileName;
    int elementIndex;

    GltfError(GltfErrorType t, const std::string &msg, const std::string &file = "", int index = -1)
        : type(t), message(msg), fileName(file), elementIndex(index) {}
};

/**
 * @brief Exception class for GLTF parsing errors
 */
class GltfParseException : public std::runtime_error
{
public:
    GltfParseException(const GltfError &error)
        : std::runtime_error(error.message), error_(error) {}

    const GltfError &getError() const { return error_; }

private:
    GltfError error_;
};

/**
 * @brief Independent GLTF/GLB format parser for OSG plugin
 *
 * Uses tinygltf library to parse GLTF/GLB files and converts them to OSG scene graph
 * Supports materials, textures, and animations conversion
 */
class GltfParser
{
public:
    GltfParser();
    ~GltfParser() = default;

    /**
     * @brief Parse GLTF/GLB model file
     * @param filePath File path
     * @return OSG scene graph root node on success, nullptr on failure
     */
    static osg::ref_ptr<osg::Group> parseFile(const std::string &filePath);

private:
    /**
     * @brief Convert GLTF model to OSG scene graph
     * @param model tinygltf model object
     * @param fileName File name
     * @return OSG scene graph root node
     */
    static osg::ref_ptr<osg::Group> convertGltfToOsg(
        const tinygltf::Model &model,
        const std::string &fileName);

    /**
     * @brief Process GLTF scene
     * @param model tinygltf model object
     * @param sceneIndex Scene index
     * @return OSG scene graph node
     */
    static osg::ref_ptr<osg::Group> processScene(const tinygltf::Model &model, int sceneIndex);

    /**
     * @brief Process GLTF node
     * @param model tinygltf model object
     * @param nodeIndex Node index
     * @return OSG node
     */
    static osg::ref_ptr<osg::Node> processNode(const tinygltf::Model &model, int nodeIndex);

    /**
     * @brief Process GLTF mesh
     * @param model tinygltf model object
     * @param meshIndex Mesh index
     * @return OSG geometry group
     */
    static osg::ref_ptr<osg::Group> processMesh(const tinygltf::Model &model, int meshIndex);

    /**
     * @brief Create OSG geometry from GLTF primitive
     * @param model tinygltf model object
     * @param primitive GLTF primitive
     * @return OSG geometry
     */
    static osg::ref_ptr<osg::Geometry> createGeometryFromPrimitive(
        const tinygltf::Model &model,
        const tinygltf::Primitive &primitive);

    /**
     * @brief Create OSG state set from GLTF material
     * @param model tinygltf model object
     * @param materialIndex Material index
     * @param materialCache Material cache for reuse
     * @param textureCache Texture cache for reuse
     * @return OSG state set
     */
    static osg::ref_ptr<osg::StateSet> createMaterialFromGltf(
        const tinygltf::Model &model,
        int materialIndex,
        std::map<int, osg::ref_ptr<osg::StateSet>> &materialCache,
        std::map<int, osg::ref_ptr<osg::Texture2D>> &textureCache);

    /**
     * @brief Create OSG texture from GLTF texture
     * @param model tinygltf model object
     * @param textureIndex Texture index
     * @param textureCache Texture cache for reuse
     * @param imageCache Image cache for reuse
     * @return OSG texture2D object
     */
    static osg::ref_ptr<osg::Texture2D> createTextureFromGltf(
        const tinygltf::Model &model,
        int textureIndex,
        std::map<int, osg::ref_ptr<osg::Texture2D>> &textureCache,
        std::map<int, osg::ref_ptr<osg::Image>> &imageCache);

    /**
     * @brief Create OSG image from GLTF image
     * @param model tinygltf model object
     * @param imageIndex Image index
     * @param imageCache Image cache for reuse
     * @return OSG image object
     */
    static osg::ref_ptr<osg::Image> createImageFromGltf(
        const tinygltf::Model &model,
        int imageIndex,
        std::map<int, osg::ref_ptr<osg::Image>> &imageCache);

    /**
     * @brief Get accessor data
     * @param model tinygltf model object
     * @param accessorIndex Accessor index
     * @return Data pointer and size
     */
    static std::pair<const void *, size_t> getAccessorData(
        const tinygltf::Model &model,
        int accessorIndex);

    /**
     * @brief Get buffer view data
     * @param model tinygltf model object
     * @param bufferViewIndex Buffer view index
     * @return Data pointer and size
     */
    static std::pair<const void *, size_t> getBufferViewData(
        const tinygltf::Model &model,
        int bufferViewIndex);

    /**
     * @brief Create OSG transform matrix from GLTF node matrix
     * @param node GLTF node
     * @return OSG matrix
     */
    static osg::Matrix createMatrixFromNode(const tinygltf::Node &node);

    /**
     * @brief Process GLTF animations
     * @param model tinygltf model object
     * @param rootGroup OSG root node group
     */
    static void processAnimations(const tinygltf::Model &model, osg::Group *rootGroup);

    /**
     * @brief Batch process geometries for performance
     * @param model tinygltf model object
     * @param primitives Primitive list
     * @param materialCache Material cache for reuse
     * @param textureCache Texture cache for reuse
     * @return Merged OSG geometry group
     */
    static osg::ref_ptr<osg::Group> batchProcessGeometries(
        const tinygltf::Model &model,
        const std::vector<tinygltf::Primitive> &primitives,
        std::map<int, osg::ref_ptr<osg::StateSet>> &materialCache,
        std::map<int, osg::ref_ptr<osg::Texture2D>> &textureCache);

    /**
     * @brief Optimize geometry data to reduce memory usage
     * @param geometry OSG geometry
     */
    static void optimizeGeometry(osg::Geometry *geometry);

    /**
     * @brief Create complete PBR material state set
     * @param model tinygltf model object
     * @param material GLTF material object
     * @param materialIndex Material index
     * @param textureCache Texture cache for reuse
     * @return OSG state set
     */
    static osg::ref_ptr<osg::StateSet> createPbrMaterial(
        const tinygltf::Model &model,
        const tinygltf::Material &material,
        int materialIndex,
        std::map<int, osg::ref_ptr<osg::Texture2D>> &textureCache);

    /**
     * @brief Process multiple texture coordinate sets
     * @param model tinygltf model object
     * @param primitive GLTF primitive
     * @param geometry OSG geometry
     */
    static void processMultipleTexCoords(
        const tinygltf::Model &model,
        const tinygltf::Primitive &primitive,
        osg::Geometry *geometry);

    /**
     * @brief Validate material parameters
     * @param material GLTF material object
     * @return Validation result and error message
     */
    static std::pair<bool, std::string> validateMaterial(const tinygltf::Material &material);

    /**
     * @brief Create image with cache
     * @param model tinygltf model object
     * @param imageIndex Image index
     * @param imageCache Image cache for reuse
     * @return OSG image object
     */
    static osg::ref_ptr<osg::Image> createImageWithCache(
        const tinygltf::Model &model,
        int imageIndex,
        std::map<int, osg::ref_ptr<osg::Image>> &imageCache);

    /**
     * @brief Process metallic roughness texture
     * @param model tinygltf model object
     * @param textureIndex Texture index
     * @param stateSet State set
     * @param textureUnit Texture unit
     * @param textureCache Texture cache for reuse
     */
    static void processMetallicRoughnessTexture(
        const tinygltf::Model &model,
        int textureIndex,
        osg::StateSet *stateSet,
        int textureUnit,
        std::map<int, osg::ref_ptr<osg::Texture2D>> &textureCache);

    /**
     * @brief Process normal texture
     * @param model tinygltf model object
     * @param normalTexture Normal texture info
     * @param stateSet State set
     * @param textureUnit Texture unit
     * @param textureCache Texture cache for reuse
     */
    static void processNormalTexture(
        const tinygltf::Model &model,
        const tinygltf::NormalTextureInfo &normalTexture,
        osg::StateSet *stateSet,
        int textureUnit,
        std::map<int, osg::ref_ptr<osg::Texture2D>> &textureCache);

    /**
     * @brief Process occlusion texture
     * @param model tinygltf model object
     * @param occlusionTexture Occlusion texture info
     * @param stateSet State set
     * @param textureUnit Texture unit
     * @param textureCache Texture cache for reuse
     */
    static void processOcclusionTexture(
        const tinygltf::Model &model,
        const tinygltf::OcclusionTextureInfo &occlusionTexture,
        osg::StateSet *stateSet,
        int textureUnit,
        std::map<int, osg::ref_ptr<osg::Texture2D>> &textureCache);

    /**
     * @brief Validate texture parameters
     * @param model tinygltf model object
     * @param textureIndex Texture index
     * @return Validation result and error message
     */
    static std::pair<bool, std::string> validateTexture(const tinygltf::Model &model, int textureIndex);

    // Error handling methods
    /**
     * @brief Validate file access and format
     * @param filePath File path to validate
     */
    static void validateFileAccess(const std::string &filePath);

    /**
     * @brief Validate GLTF model structure
     * @param model tinygltf model object
     * @param fileName File name for error reporting
     */
    static void validateModel(const tinygltf::Model &model, const std::string &fileName);

    /**
     * @brief Validate scene index
     * @param model tinygltf model object
     * @param sceneIndex Scene index to validate
     */
    static void validateSceneIndex(const tinygltf::Model &model, int sceneIndex);

    /**
     * @brief Validate node index
     * @param model tinygltf model object
     * @param nodeIndex Node index to validate
     */
    static void validateNodeIndex(const tinygltf::Model &model, int nodeIndex);

    /**
     * @brief Validate mesh index
     * @param model tinygltf model object
     * @param meshIndex Mesh index to validate
     */
    static void validateMeshIndex(const tinygltf::Model &model, int meshIndex);

    /**
     * @brief Validate material index
     * @param model tinygltf model object
     * @param materialIndex Material index to validate
     */
    static void validateMaterialIndex(const tinygltf::Model &model, int materialIndex);

    /**
     * @brief Validate accessor index and data
     * @param model tinygltf model object
     * @param accessorIndex Accessor index to validate
     */
    static void validateAccessorIndex(const tinygltf::Model &model, int accessorIndex);

    /**
     * @brief Get error type string
     * @param type Error type
     * @return Error type string
     */
    static std::string getErrorTypeString(GltfErrorType type);

    /**
     * @brief Log error with details
     * @param type Error type
     * @param message Error message
     * @param fileName File name
     * @param elementIndex Element index (-1 if not applicable)
     */
    static void logError(GltfErrorType type, const std::string &message,
                         const std::string &fileName = "", int elementIndex = -1);
};

#endif // GLTFPARSER_H