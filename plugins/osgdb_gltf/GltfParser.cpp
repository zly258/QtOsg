#include "GltfParser.h"
#include <osg/Array>
#include <osg/Geometry>
#include <osg/PrimitiveSet>
#include <osg/Image>
#include <osg/TexGen>
#include <osg/TexEnv>
#include <osg/Material>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osg/BlendFunc>
#include <osg/AlphaFunc>
#include <osgUtil/SmoothingVisitor>
#include <osgDB/ReadFile>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <filesystem>

GltfParser::GltfParser()
{
}

osg::ref_ptr<osg::Group> GltfParser::parseFile(
    const std::string &filePath)
{

    auto startTime = std::chrono::high_resolution_clock::now();

    try
    {
        // Validate file access first
        validateFileAccess(filePath);

        // Check file extension
        std::string extension;
        size_t dotPos = filePath.find_last_of('.');
        if (dotPos != std::string::npos)
        {
            extension = filePath.substr(dotPos + 1);
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        }

        if (extension != "gltf" && extension != "glb")
        {
            throw GltfParseException(GltfError(GltfErrorType::UNSUPPORTED_FORMAT,
                                               "Unsupported file format: " + extension, filePath));
        }

        PluginLogger::logFileLoadStart("GLTF", filePath);

        tinygltf::Model model;
        tinygltf::TinyGLTF loader;
        std::string err;
        std::string warn;

        bool ret = false;

        if (extension == "gltf")
        {
            // Load ASCII format GLTF file
            ret = loader.LoadASCIIFromFile(&model, &err, &warn, filePath);
        }
        else if (extension == "glb")
        {
            // Load binary format GLB file
            ret = loader.LoadBinaryFromFile(&model, &err, &warn, filePath);
        }

        if (!warn.empty())
        {
            PluginLogger::logWarning("GLTF", "TinyGLTF warning: " + warn);
        }

        if (!err.empty())
        {
            throw GltfParseException(GltfError(GltfErrorType::TINYGLTF_ERROR,
                                               "TinyGLTF error: " + err, filePath));
        }

        if (!ret)
        {
            throw GltfParseException(GltfError(GltfErrorType::CORRUPTED_DATA,
                                               "Failed to parse GLTF file", filePath));
        }

        // Validate loaded model
        validateModel(model, filePath);

        // Extract filename without path and extension
        std::string fileName = filePath;
        size_t slashPos = fileName.find_last_of("/\\");
        if (slashPos != std::string::npos)
        {
            fileName = fileName.substr(slashPos + 1);
        }
        size_t dotPos2 = fileName.find_last_of('.');
        if (dotPos2 != std::string::npos)
        {
            fileName = fileName.substr(0, dotPos2);
        }

        // Convert to OSG scene graph
        osg::ref_ptr<osg::Group> rootGroup = convertGltfToOsg(model, fileName);

        if (!rootGroup.valid())
        {
            throw GltfParseException(GltfError(GltfErrorType::CORRUPTED_DATA,
                                               "GLTF to OSG conversion failed", filePath));
        }

        // Log successful loading with statistics
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        std::ostringstream stats;
        stats << "Successfully loaded " << filePath << " in " << duration.count() << "ms"
              << " - Scenes: " << model.scenes.size()
              << ", Nodes: " << model.nodes.size()
              << ", Meshes: " << model.meshes.size()
              << ", Materials: " << model.materials.size()
              << ", Textures: " << model.textures.size()
              << ", Animations: " << model.animations.size();

        PluginLogger::logInfo("GLTF", stats.str());

        return rootGroup;
    }
    catch (const GltfParseException &e)
    {
        const GltfError &error = e.getError();
        PluginLogger::logFileLoadFailure("GLTF", filePath,
                                         getErrorTypeString(error.type) + ": " + error.message);
        return nullptr;
    }
    catch (const std::exception &e)
    {
        logError(GltfErrorType::UNKNOWN_ERROR, std::string("Standard exception: ") + e.what(), filePath);
        return nullptr;
    }
    catch (...)
    {
        logError(GltfErrorType::UNKNOWN_ERROR, "Unknown exception occurred", filePath);
        return nullptr;
    }
}

osg::ref_ptr<osg::Group> GltfParser::convertGltfToOsg(
    const tinygltf::Model &model,
    const std::string &fileName)
{

    osg::ref_ptr<osg::Group> rootGroup = new osg::Group();
    rootGroup->setName(fileName);

    // Process default scene or first scene
    int sceneIndex = model.defaultScene >= 0 ? model.defaultScene : 0;

    if (sceneIndex >= 0 && sceneIndex < static_cast<int>(model.scenes.size()))
    {

        osg::ref_ptr<osg::Group> sceneGroup = processScene(model, sceneIndex);
        if (sceneGroup.valid())
        {
            rootGroup->addChild(sceneGroup);
        }
    }

    // Process animations
    if (!model.animations.empty())
    {
        processAnimations(model, rootGroup.get());
    }

    return rootGroup;
}

osg::ref_ptr<osg::Group> GltfParser::processScene(const tinygltf::Model &model, int sceneIndex)
{
    if (sceneIndex < 0 || sceneIndex >= static_cast<int>(model.scenes.size()))
    {
        return nullptr;
    }

    const tinygltf::Scene &scene = model.scenes[sceneIndex];
    osg::ref_ptr<osg::Group> sceneGroup = new osg::Group();

    if (!scene.name.empty())
    {
        sceneGroup->setName(scene.name);
    }
    else
    {
        sceneGroup->setName("Scene_" + std::to_string(sceneIndex));
    }

    // Process root nodes in scene
    for (int nodeIndex : scene.nodes)
    {
        osg::ref_ptr<osg::Node> node = processNode(model, nodeIndex);
        if (node.valid())
        {
            sceneGroup->addChild(node);
        }
    }

    return sceneGroup;
}

osg::ref_ptr<osg::Node> GltfParser::processNode(const tinygltf::Model &model, int nodeIndex)
{
    if (nodeIndex < 0 || nodeIndex >= static_cast<int>(model.nodes.size()))
    {
        return nullptr;
    }

    const tinygltf::Node &gltfNode = model.nodes[nodeIndex];

    // Create transform node
    osg::ref_ptr<osg::MatrixTransform> transform = new osg::MatrixTransform();

    if (!gltfNode.name.empty())
    {
        transform->setName(gltfNode.name);
    }
    else
    {
        transform->setName("Node_" + std::to_string(nodeIndex));
    }

    // Set transform matrix
    osg::Matrix matrix = createMatrixFromNode(gltfNode);
    transform->setMatrix(matrix);

    // Process mesh
    if (gltfNode.mesh >= 0)
    {
        osg::ref_ptr<osg::Group> meshGroup = processMesh(model, gltfNode.mesh);
        if (meshGroup.valid())
        {
            transform->addChild(meshGroup);
        }
    }

    // Recursively process child nodes
    for (int childIndex : gltfNode.children)
    {
        osg::ref_ptr<osg::Node> childNode = processNode(model, childIndex);
        if (childNode.valid())
        {
            transform->addChild(childNode);
        }
    }

    return transform;
}
osg::ref_ptr<osg::Group> GltfParser::processMesh(const tinygltf::Model &model, int meshIndex)
{
    if (meshIndex < 0 || meshIndex >= static_cast<int>(model.meshes.size()))
    {
        return nullptr;
    }

    const tinygltf::Mesh &mesh = model.meshes[meshIndex];
    osg::ref_ptr<osg::Group> meshGroup = new osg::Group();

    if (!mesh.name.empty())
    {
        meshGroup->setName(mesh.name);
    }
    else
    {
        meshGroup->setName("Mesh_" + std::to_string(meshIndex));
    }

    // Create caches for this mesh processing
    std::map<int, osg::ref_ptr<osg::StateSet>> materialCache;
    std::map<int, osg::ref_ptr<osg::Texture2D>> textureCache;

    // Optimization: if primitive count is high, use batch processing
    if (mesh.primitives.size() > 5)
    {
        osg::ref_ptr<osg::Group> batchedGroup = batchProcessGeometries(model, mesh.primitives, materialCache, textureCache);
        if (batchedGroup.valid())
        {
            meshGroup->addChild(batchedGroup);
        }
    }
    else
    {
        // Process each primitive in the mesh
        for (size_t i = 0; i < mesh.primitives.size(); ++i)
        {
            const tinygltf::Primitive &primitive = mesh.primitives[i];

            osg::ref_ptr<osg::Geometry> geometry = createGeometryFromPrimitive(model, primitive);
            if (geometry.valid())
            {
                // Apply geometry optimization
                optimizeGeometry(geometry.get());

                osg::ref_ptr<osg::Geode> geode = new osg::Geode();
                geode->addDrawable(geometry);

                // Apply material
                if (primitive.material >= 0)
                {
                    osg::ref_ptr<osg::StateSet> stateSet = createMaterialFromGltf(model, primitive.material, materialCache, textureCache);
                    if (stateSet.valid())
                    {
                        geode->setStateSet(stateSet);
                    }
                }

                meshGroup->addChild(geode);
            }
        }
    }

    return meshGroup;
}

osg::ref_ptr<osg::Geometry> GltfParser::createGeometryFromPrimitive(
    const tinygltf::Model &model,
    const tinygltf::Primitive &primitive)
{

    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();

    // Process vertex positions
    auto positionIt = primitive.attributes.find("POSITION");
    if (positionIt != primitive.attributes.end())
    {
        int accessorIndex = positionIt->second;
        auto [data, count] = getAccessorData(model, accessorIndex);
        if (data && count > 0)
        {
            osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
            const float *floatData = static_cast<const float *>(data);

            for (size_t i = 0; i < count; ++i)
            {
                vertices->push_back(osg::Vec3(
                    floatData[i * 3 + 0],
                    floatData[i * 3 + 1],
                    floatData[i * 3 + 2]));
            }
            geometry->setVertexArray(vertices.get());
        }
    }

    // Process normals
    auto normalIt = primitive.attributes.find("NORMAL");
    if (normalIt != primitive.attributes.end())
    {
        int accessorIndex = normalIt->second;
        auto [data, count] = getAccessorData(model, accessorIndex);
        if (data && count > 0)
        {
            osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
            const float *floatData = static_cast<const float *>(data);

            for (size_t i = 0; i < count; ++i)
            {
                normals->push_back(osg::Vec3(
                    floatData[i * 3 + 0],
                    floatData[i * 3 + 1],
                    floatData[i * 3 + 2]));
            }
            geometry->setNormalArray(normals.get());
            geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
        }
    }

    // Process multiple texture coordinate sets
    processMultipleTexCoords(model, primitive, geometry.get());

    // Process vertex colors
    auto colorIt = primitive.attributes.find("COLOR_0");
    if (colorIt != primitive.attributes.end())
    {
        int accessorIndex = colorIt->second;
        auto [data, count] = getAccessorData(model, accessorIndex);
        if (data && count > 0)
        {
            const tinygltf::Accessor &accessor = model.accessors[accessorIndex];
            osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();

            if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
            {
                const float *floatData = static_cast<const float *>(data);
                int componentCount = (accessor.type == TINYGLTF_TYPE_VEC3) ? 3 : 4;

                for (size_t i = 0; i < count; ++i)
                {
                    if (componentCount == 3)
                    {
                        colors->push_back(osg::Vec4(
                            floatData[i * 3 + 0],
                            floatData[i * 3 + 1],
                            floatData[i * 3 + 2],
                            1.0f));
                    }
                    else
                    {
                        colors->push_back(osg::Vec4(
                            floatData[i * 4 + 0],
                            floatData[i * 4 + 1],
                            floatData[i * 4 + 2],
                            floatData[i * 4 + 3]));
                    }
                }
            }
            else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
            {
                const unsigned char *byteData = static_cast<const unsigned char *>(data);
                int componentCount = (accessor.type == TINYGLTF_TYPE_VEC3) ? 3 : 4;

                for (size_t i = 0; i < count; ++i)
                {
                    if (componentCount == 3)
                    {
                        colors->push_back(osg::Vec4(
                            byteData[i * 3 + 0] / 255.0f,
                            byteData[i * 3 + 1] / 255.0f,
                            byteData[i * 3 + 2] / 255.0f,
                            1.0f));
                    }
                    else
                    {
                        colors->push_back(osg::Vec4(
                            byteData[i * 4 + 0] / 255.0f,
                            byteData[i * 4 + 1] / 255.0f,
                            byteData[i * 4 + 2] / 255.0f,
                            byteData[i * 4 + 3] / 255.0f));
                    }
                }
            }

            geometry->setColorArray(colors.get());
            geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
        }
    }

    // Process indices
    if (primitive.indices >= 0)
    {
        auto [indexData, indexCount] = getAccessorData(model, primitive.indices);
        if (indexData && indexCount > 0)
        {
            const tinygltf::Accessor &indexAccessor = model.accessors[primitive.indices];

            osg::ref_ptr<osg::DrawElementsUInt> drawElements = new osg::DrawElementsUInt();

            // Set draw mode based on primitive mode
            GLenum mode = GL_TRIANGLES;
            switch (primitive.mode)
            {
            case TINYGLTF_MODE_POINTS:
                mode = GL_POINTS;
                break;
            case TINYGLTF_MODE_LINE:
                mode = GL_LINES;
                break;
            case TINYGLTF_MODE_LINE_LOOP:
                mode = GL_LINE_LOOP;
                break;
            case TINYGLTF_MODE_LINE_STRIP:
                mode = GL_LINE_STRIP;
                break;
            case TINYGLTF_MODE_TRIANGLES:
                mode = GL_TRIANGLES;
                break;
            case TINYGLTF_MODE_TRIANGLE_STRIP:
                mode = GL_TRIANGLE_STRIP;
                break;
            case TINYGLTF_MODE_TRIANGLE_FAN:
                mode = GL_TRIANGLE_FAN;
                break;
            default:
                mode = GL_TRIANGLES;
                break;
            }
            drawElements->setMode(mode);

            // Read indices based on data type
            if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
            {
                const unsigned short *shortData = static_cast<const unsigned short *>(indexData);
                for (size_t i = 0; i < indexCount; ++i)
                {
                    drawElements->push_back(shortData[i]);
                }
            }
            else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
            {
                const unsigned int *intData = static_cast<const unsigned int *>(indexData);
                for (size_t i = 0; i < indexCount; ++i)
                {
                    drawElements->push_back(intData[i]);
                }
            }
            else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
            {
                const unsigned char *byteData = static_cast<const unsigned char *>(indexData);
                for (size_t i = 0; i < indexCount; ++i)
                {
                    drawElements->push_back(byteData[i]);
                }
            }

            geometry->addPrimitiveSet(drawElements.get());
        }
    }
    else
    {
        // No indices, draw directly using vertex array
        osg::Vec3Array *vertices = dynamic_cast<osg::Vec3Array *>(geometry->getVertexArray());
        if (vertices && vertices->size() > 0)
        {
            GLenum mode = GL_TRIANGLES;
            switch (primitive.mode)
            {
            case TINYGLTF_MODE_POINTS:
                mode = GL_POINTS;
                break;
            case TINYGLTF_MODE_LINE:
                mode = GL_LINES;
                break;
            case TINYGLTF_MODE_LINE_LOOP:
                mode = GL_LINE_LOOP;
                break;
            case TINYGLTF_MODE_LINE_STRIP:
                mode = GL_LINE_STRIP;
                break;
            case TINYGLTF_MODE_TRIANGLES:
                mode = GL_TRIANGLES;
                break;
            case TINYGLTF_MODE_TRIANGLE_STRIP:
                mode = GL_TRIANGLE_STRIP;
                break;
            case TINYGLTF_MODE_TRIANGLE_FAN:
                mode = GL_TRIANGLE_FAN;
                break;
            default:
                mode = GL_TRIANGLES;
                break;
            }

            geometry->addPrimitiveSet(new osg::DrawArrays(mode, 0, vertices->size()));
        }
    }

    // If no normals, automatically calculate normals
    if (!geometry->getNormalArray())
    {
        osgUtil::SmoothingVisitor::smooth(*geometry);
    }

    return geometry;
}

osg::ref_ptr<osg::StateSet> GltfParser::createMaterialFromGltf(
    const tinygltf::Model &model,
    int materialIndex,
    std::map<int, osg::ref_ptr<osg::StateSet>> &materialCache,
    std::map<int, osg::ref_ptr<osg::Texture2D>> &textureCache)
{

    if (materialIndex < 0 || materialIndex >= static_cast<int>(model.materials.size()))
    {
        // Return default material
        osg::ref_ptr<osg::StateSet> stateSet = new osg::StateSet();
        osg::ref_ptr<osg::Material> material = new osg::Material();
        material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(0.8f, 0.8f, 0.8f, 1.0f));
        stateSet->setAttributeAndModes(material, osg::StateAttribute::ON);
        return stateSet;
    }

    // Check cache
    auto cacheIt = materialCache.find(materialIndex);
    if (cacheIt != materialCache.end())
    {
        return cacheIt->second;
    }

    const tinygltf::Material &gltfMaterial = model.materials[materialIndex];

    // Validate material parameters
    auto [isValid, errorMsg] = validateMaterial(gltfMaterial);
    if (!isValid)
    {
        std::cerr << "GltfParser: Material validation failed: " << errorMsg << std::endl;
        // Continue with default values
    }

    // Use enhanced PBR material creation method
    std::map<int, osg::ref_ptr<osg::Image>> imageCache;
    osg::ref_ptr<osg::StateSet> stateSet = createPbrMaterial(model, gltfMaterial, materialIndex, textureCache);

    // Cache material
    materialCache[materialIndex] = stateSet;

    return stateSet;
}

osg::ref_ptr<osg::Texture2D> GltfParser::createTextureFromGltf(
    const tinygltf::Model &model,
    int textureIndex,
    std::map<int, osg::ref_ptr<osg::Texture2D>> &textureCache,
    std::map<int, osg::ref_ptr<osg::Image>> &imageCache)
{

    if (textureIndex < 0 || textureIndex >= static_cast<int>(model.textures.size()))
    {
        return nullptr;
    }

    // Check cache
    auto cacheIt = textureCache.find(textureIndex);
    if (cacheIt != textureCache.end())
    {
        return cacheIt->second;
    }

    const tinygltf::Texture &gltfTexture = model.textures[textureIndex];

    // Create image (using cache)
    osg::ref_ptr<osg::Image> image = createImageWithCache(model, gltfTexture.source, imageCache);
    if (!image.valid())
    {
        return nullptr;
    }

    // Create texture
    osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D();
    texture->setImage(image.get());

    // Set texture parameters
    if (gltfTexture.sampler >= 0 && gltfTexture.sampler < static_cast<int>(model.samplers.size()))
    {
        const tinygltf::Sampler &sampler = model.samplers[gltfTexture.sampler];

        // Set filter modes
        switch (sampler.minFilter)
        {
        case TINYGLTF_TEXTURE_FILTER_NEAREST:
            texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
            break;
        case TINYGLTF_TEXTURE_FILTER_LINEAR:
            texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
            break;
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
            texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST_MIPMAP_NEAREST);
            break;
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
            texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR_MIPMAP_NEAREST);
            break;
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
            texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST_MIPMAP_LINEAR);
            break;
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
            texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR_MIPMAP_LINEAR);
            break;
        default:
            texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
            break;
        }

        switch (sampler.magFilter)
        {
        case TINYGLTF_TEXTURE_FILTER_NEAREST:
            texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
            break;
        case TINYGLTF_TEXTURE_FILTER_LINEAR:
            texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
            break;
        default:
            texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
            break;
        }

        // Set wrap modes
        switch (sampler.wrapS)
        {
        case TINYGLTF_TEXTURE_WRAP_REPEAT:
            texture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
            break;
        case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
            texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            break;
        case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
            texture->setWrap(osg::Texture::WRAP_S, osg::Texture::MIRROR);
            break;
        default:
            texture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
            break;
        }

        switch (sampler.wrapT)
        {
        case TINYGLTF_TEXTURE_WRAP_REPEAT:
            texture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
            break;
        case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
            texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            break;
        case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
            texture->setWrap(osg::Texture::WRAP_T, osg::Texture::MIRROR);
            break;
        default:
            texture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
            break;
        }
    }
    else
    {
        // Default texture parameters
        texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        texture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        texture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    }

    // Cache texture
    textureCache[textureIndex] = texture;

    return texture;
}

osg::ref_ptr<osg::Image> GltfParser::createImageFromGltf(
    const tinygltf::Model &model,
    int imageIndex,
    std::map<int, osg::ref_ptr<osg::Image>> &imageCache)
{

    if (imageIndex < 0 || imageIndex >= static_cast<int>(model.images.size()))
    {
        return nullptr;
    }

    const tinygltf::Image &gltfImage = model.images[imageIndex];

    // If image data is embedded in GLTF file
    if (!gltfImage.image.empty())
    {
        osg::ref_ptr<osg::Image> image = new osg::Image();

        // Determine image format
        GLenum pixelFormat = GL_RGB;
        GLenum dataType = GL_UNSIGNED_BYTE;
        GLint internalFormat = GL_RGB;

        if (gltfImage.component == 1)
        {
            pixelFormat = GL_LUMINANCE;
            internalFormat = GL_LUMINANCE;
        }
        else if (gltfImage.component == 2)
        {
            pixelFormat = GL_LUMINANCE_ALPHA;
            internalFormat = GL_LUMINANCE_ALPHA;
        }
        else if (gltfImage.component == 3)
        {
            pixelFormat = GL_RGB;
            internalFormat = GL_RGB;
        }
        else if (gltfImage.component == 4)
        {
            pixelFormat = GL_RGBA;
            internalFormat = GL_RGBA;
        }

        // Set image data
        image->setImage(
            gltfImage.width,
            gltfImage.height,
            1, // depth
            internalFormat,
            pixelFormat,
            dataType,
            const_cast<unsigned char *>(gltfImage.image.data()),
            osg::Image::NO_DELETE // Don't delete data, it belongs to tinygltf
        );

        // Copy data to avoid lifecycle issues
        image->dirty();

        return image;
    }

    // If image is external file reference
    if (!gltfImage.uri.empty())
    {
        // Check if it's a data URI (base64 encoded)
        if (gltfImage.uri.find("data:") == 0)
        {
            // Handle data URI
            size_t commaPos = gltfImage.uri.find(',');
            if (commaPos != std::string::npos)
            {
                std::string base64Data = gltfImage.uri.substr(commaPos + 1);

                // Base64 decoding needed here, but simplified for now
                std::cerr << "GltfParser: Base64 encoded texture data URI not supported yet" << std::endl;
                return nullptr;
            }
        }
        else
        {
            // External file reference
            std::string imagePath = gltfImage.uri;

            // If relative path, need to be relative to GLTF file directory
            // For simplification, try to load directly
            std::cout << "GltfParser: Attempting to load external texture file: " << imagePath << std::endl;

            osg::ref_ptr<osg::Image> image = osgDB::readImageFile(imagePath);

            if (!image.valid())
            {
                std::cerr << "GltfParser: Cannot load texture file: " << imagePath << std::endl;
                return nullptr;
            }

            return image;
        }
    }

    // If image references a bufferView
    if (gltfImage.bufferView >= 0)
    {
        auto [data, size] = getBufferViewData(model, gltfImage.bufferView);
        if (data && size > 0)
        {
            // Create temporary file or use memory stream to decode image
            // Simplified handling, return null for now
            std::cerr << "GltfParser: Loading texture from bufferView not supported yet" << std::endl;
            return nullptr;
        }
    }

    return nullptr;
}

std::pair<const void *, size_t> GltfParser::getAccessorData(
    const tinygltf::Model &model,
    int accessorIndex)
{

    if (accessorIndex < 0 || accessorIndex >= static_cast<int>(model.accessors.size()))
    {
        return {nullptr, 0};
    }

    const tinygltf::Accessor &accessor = model.accessors[accessorIndex];

    if (accessor.bufferView < 0 || accessor.bufferView >= static_cast<int>(model.bufferViews.size()))
    {
        return {nullptr, 0};
    }

    const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];

    if (bufferView.buffer < 0 || bufferView.buffer >= static_cast<int>(model.buffers.size()))
    {
        return {nullptr, 0};
    }

    const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];

    if (buffer.data.empty())
    {
        return {nullptr, 0};
    }

    // Calculate data offset
    size_t offset = bufferView.byteOffset + accessor.byteOffset;

    if (offset >= buffer.data.size())
    {
        return {nullptr, 0};
    }

    // Return data pointer and element count
    const void *data = buffer.data.data() + offset;
    size_t count = accessor.count;

    return {data, count};
}

std::pair<const void *, size_t> GltfParser::getBufferViewData(
    const tinygltf::Model &model,
    int bufferViewIndex)
{

    if (bufferViewIndex < 0 || bufferViewIndex >= static_cast<int>(model.bufferViews.size()))
    {
        return {nullptr, 0};
    }

    const tinygltf::BufferView &bufferView = model.bufferViews[bufferViewIndex];

    if (bufferView.buffer < 0 || bufferView.buffer >= static_cast<int>(model.buffers.size()))
    {
        return {nullptr, 0};
    }

    const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];

    if (buffer.data.empty())
    {
        return {nullptr, 0};
    }

    if (bufferView.byteOffset >= buffer.data.size())
    {
        return {nullptr, 0};
    }

    const void *data = buffer.data.data() + bufferView.byteOffset;
    size_t size = bufferView.byteLength;

    return {data, size};
}

osg::Matrix GltfParser::createMatrixFromNode(const tinygltf::Node &node)
{
    osg::Matrix matrix;

    if (node.matrix.size() == 16)
    {
        // Use matrix directly
        matrix.set(
            node.matrix[0], node.matrix[1], node.matrix[2], node.matrix[3],
            node.matrix[4], node.matrix[5], node.matrix[6], node.matrix[7],
            node.matrix[8], node.matrix[9], node.matrix[10], node.matrix[11],
            node.matrix[12], node.matrix[13], node.matrix[14], node.matrix[15]);
    }
    else
    {
        // Build matrix from TRS
        osg::Vec3 translation(0, 0, 0);
        osg::Quat rotation(0, 0, 0, 1);
        osg::Vec3 scale(1, 1, 1);

        if (node.translation.size() == 3)
        {
            translation.set(node.translation[0], node.translation[1], node.translation[2]);
        }

        if (node.rotation.size() == 4)
        {
            rotation.set(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]);
        }

        if (node.scale.size() == 3)
        {
            scale.set(node.scale[0], node.scale[1], node.scale[2]);
        }

        // Build matrix from TRS components
        matrix = osg::Matrix::scale(scale) *
                 osg::Matrix::rotate(rotation) *
                 osg::Matrix::translate(translation);
    }

    return matrix;
}

void GltfParser::processAnimations(const tinygltf::Model &model, osg::Group *rootGroup)
{
    // Animation processing is complex and optional for basic functionality
    // For now, we'll skip animation processing to focus on core geometry loading
    if (!model.animations.empty())
    {
        std::cout << "GltfParser: Animation processing not implemented yet. Found "
                  << model.animations.size() << " animations." << std::endl;
    }
}

osg::ref_ptr<osg::Group> GltfParser::batchProcessGeometries(
    const tinygltf::Model &model,
    const std::vector<tinygltf::Primitive> &primitives,
    std::map<int, osg::ref_ptr<osg::StateSet>> &materialCache,
    std::map<int, osg::ref_ptr<osg::Texture2D>> &textureCache)
{

    // For now, use simple processing instead of batching
    // Batching optimization can be implemented later
    osg::ref_ptr<osg::Group> group = new osg::Group();

    for (size_t i = 0; i < primitives.size(); ++i)
    {
        const tinygltf::Primitive &primitive = primitives[i];

        osg::ref_ptr<osg::Geometry> geometry = createGeometryFromPrimitive(model, primitive);
        if (geometry.valid())
        {
            optimizeGeometry(geometry.get());

            osg::ref_ptr<osg::Geode> geode = new osg::Geode();
            geode->addDrawable(geometry);

            if (primitive.material >= 0)
            {
                osg::ref_ptr<osg::StateSet> stateSet = createMaterialFromGltf(model, primitive.material, materialCache, textureCache);
                if (stateSet.valid())
                {
                    geode->setStateSet(stateSet);
                }
            }

            group->addChild(geode);
        }
    }

    return group;
}

void GltfParser::optimizeGeometry(osg::Geometry *geometry)
{
    if (!geometry)
        return;

    // Basic geometry optimization
    // More advanced optimizations can be added later
    geometry->setUseDisplayList(true);
    geometry->setUseVertexBufferObjects(true);
}

osg::ref_ptr<osg::StateSet> GltfParser::createPbrMaterial(
    const tinygltf::Model &model,
    const tinygltf::Material &material,
    int materialIndex,
    std::map<int, osg::ref_ptr<osg::Texture2D>> &textureCache)
{

    osg::ref_ptr<osg::StateSet> stateSet = new osg::StateSet();
    osg::ref_ptr<osg::Material> osgMaterial = new osg::Material();

    // Set material name
    if (!material.name.empty())
    {
        stateSet->setName(material.name);
    }
    else
    {
        stateSet->setName("Material_" + std::to_string(materialIndex));
    }

    int textureUnit = 0;

    // Process PBR metallic roughness
    if (material.pbrMetallicRoughness.baseColorTexture.index >= 0)
    {
        auto [isValid, errorMsg] = validateTexture(model, material.pbrMetallicRoughness.baseColorTexture.index);
        if (isValid)
        {
            std::map<int, osg::ref_ptr<osg::Image>> imageCache;
            osg::ref_ptr<osg::Texture2D> baseColorTexture = createTextureFromGltf(
                model,
                material.pbrMetallicRoughness.baseColorTexture.index,
                textureCache,
                imageCache);
            if (baseColorTexture.valid())
            {
                stateSet->setTextureAttributeAndModes(textureUnit, baseColorTexture, osg::StateAttribute::ON);
                textureUnit++;
            }
        }
        else
        {
            std::cerr << "GltfParser: Base color texture validation failed: " << errorMsg << std::endl;
        }
    }

    // Process metallic roughness texture
    if (material.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0)
    {
        processMetallicRoughnessTexture(model,
                                        material.pbrMetallicRoughness.metallicRoughnessTexture.index,
                                        stateSet.get(), textureUnit, textureCache);
        textureUnit++;
    }

    // Process normal map
    if (material.normalTexture.index >= 0)
    {
        processNormalTexture(model, material.normalTexture, stateSet.get(), textureUnit, textureCache);
        textureUnit++;
    }

    // Process occlusion map
    if (material.occlusionTexture.index >= 0)
    {
        processOcclusionTexture(model, material.occlusionTexture, stateSet.get(), textureUnit, textureCache);
        textureUnit++;
    }

    // Process emissive texture
    if (material.emissiveTexture.index >= 0)
    {
        auto [isValid, errorMsg] = validateTexture(model, material.emissiveTexture.index);
        if (isValid)
        {
            std::map<int, osg::ref_ptr<osg::Image>> imageCache;
            osg::ref_ptr<osg::Texture2D> emissiveTexture = createTextureFromGltf(
                model,
                material.emissiveTexture.index,
                textureCache,
                imageCache);
            if (emissiveTexture.valid())
            {
                stateSet->setTextureAttributeAndModes(textureUnit, emissiveTexture, osg::StateAttribute::ON);
                textureUnit++;
            }
        }
        else
        {
            std::cerr << "GltfParser: Emissive texture validation failed: " << errorMsg << std::endl;
        }
    }

    // Set base color factor
    const auto &baseColorFactor = material.pbrMetallicRoughness.baseColorFactor;
    if (baseColorFactor.size() >= 3)
    {
        osg::Vec4 color(
            baseColorFactor[0],
            baseColorFactor[1],
            baseColorFactor[2],
            baseColorFactor.size() >= 4 ? baseColorFactor[3] : 1.0f);
        osgMaterial->setDiffuse(osg::Material::FRONT_AND_BACK, color);
        osgMaterial->setAmbient(osg::Material::FRONT_AND_BACK, color * 0.2f);
    }
    else
    {
        // Default white color
        osgMaterial->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
        osgMaterial->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(0.2f, 0.2f, 0.2f, 1.0f));
    }

    // Set metallic and roughness factors (simplified)
    float metallicFactor = material.pbrMetallicRoughness.metallicFactor;
    float roughnessFactor = material.pbrMetallicRoughness.roughnessFactor;

    // Convert metallic/roughness to OSG material properties (approximation)
    float shininess = (1.0f - roughnessFactor) * 128.0f;
    osgMaterial->setShininess(osg::Material::FRONT_AND_BACK, shininess);

    osg::Vec4 specular = osg::Vec4(metallicFactor, metallicFactor, metallicFactor, 1.0f);
    osgMaterial->setSpecular(osg::Material::FRONT_AND_BACK, specular);

    // Set emissive factor
    const auto &emissiveFactor = material.emissiveFactor;
    if (emissiveFactor.size() >= 3)
    {
        osg::Vec4 emissive(
            emissiveFactor[0],
            emissiveFactor[1],
            emissiveFactor[2],
            1.0f);
        osgMaterial->setEmission(osg::Material::FRONT_AND_BACK, emissive);
    }

    // Handle alpha mode
    if (material.alphaMode == "BLEND")
    {
        stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
        stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

        osg::ref_ptr<osg::BlendFunc> blendFunc = new osg::BlendFunc();
        blendFunc->setFunction(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        stateSet->setAttributeAndModes(blendFunc, osg::StateAttribute::ON);
    }
    else if (material.alphaMode == "MASK")
    {
        osg::ref_ptr<osg::AlphaFunc> alphaFunc = new osg::AlphaFunc();
        alphaFunc->setFunction(osg::AlphaFunc::GREATER, material.alphaCutoff);
        stateSet->setAttributeAndModes(alphaFunc, osg::StateAttribute::ON);
    }

    // Handle double sided
    if (material.doubleSided)
    {
        stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    }
    else
    {
        stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
        osg::ref_ptr<osg::CullFace> cullFace = new osg::CullFace();
        cullFace->setMode(osg::CullFace::BACK);
        stateSet->setAttributeAndModes(cullFace, osg::StateAttribute::ON);
    }

    stateSet->setAttributeAndModes(osgMaterial, osg::StateAttribute::ON);

    return stateSet;
}

void GltfParser::processMultipleTexCoords(
    const tinygltf::Model &model,
    const tinygltf::Primitive &primitive,
    osg::Geometry *geometry)
{

    // Process TEXCOORD_0
    auto texCoord0It = primitive.attributes.find("TEXCOORD_0");
    if (texCoord0It != primitive.attributes.end())
    {
        int accessorIndex = texCoord0It->second;
        auto [data, count] = getAccessorData(model, accessorIndex);
        if (data && count > 0)
        {
            osg::ref_ptr<osg::Vec2Array> texCoords = new osg::Vec2Array();
            const float *floatData = static_cast<const float *>(data);

            for (size_t i = 0; i < count; ++i)
            {
                texCoords->push_back(osg::Vec2(
                    floatData[i * 2 + 0],
                    floatData[i * 2 + 1]));
            }
            geometry->setTexCoordArray(0, texCoords.get());
        }
    }

    // Process TEXCOORD_1 if present
    auto texCoord1It = primitive.attributes.find("TEXCOORD_1");
    if (texCoord1It != primitive.attributes.end())
    {
        int accessorIndex = texCoord1It->second;
        auto [data, count] = getAccessorData(model, accessorIndex);
        if (data && count > 0)
        {
            osg::ref_ptr<osg::Vec2Array> texCoords = new osg::Vec2Array();
            const float *floatData = static_cast<const float *>(data);

            for (size_t i = 0; i < count; ++i)
            {
                texCoords->push_back(osg::Vec2(
                    floatData[i * 2 + 0],
                    floatData[i * 2 + 1]));
            }
            geometry->setTexCoordArray(1, texCoords.get());
        }
    }
}

std::pair<bool, std::string> GltfParser::validateMaterial(const tinygltf::Material &material)
{
    // Basic material validation
    // More comprehensive validation can be added later

    // Check base color factor range
    const auto &baseColorFactor = material.pbrMetallicRoughness.baseColorFactor;
    for (size_t i = 0; i < baseColorFactor.size(); ++i)
    {
        if (baseColorFactor[i] < 0.0 || baseColorFactor[i] > 1.0)
        {
            return {false, "Base color factor out of range [0,1]"};
        }
    }

    // Check metallic factor range
    if (material.pbrMetallicRoughness.metallicFactor < 0.0 ||
        material.pbrMetallicRoughness.metallicFactor > 1.0)
    {
        return {false, "Metallic factor out of range [0,1]"};
    }

    // Check roughness factor range
    if (material.pbrMetallicRoughness.roughnessFactor < 0.0 ||
        material.pbrMetallicRoughness.roughnessFactor > 1.0)
    {
        return {false, "Roughness factor out of range [0,1]"};
    }

    // Check alpha cutoff range
    if (material.alphaCutoff < 0.0 || material.alphaCutoff > 1.0)
    {
        return {false, "Alpha cutoff out of range [0,1]"};
    }

    return {true, ""};
}

osg::ref_ptr<osg::Image> GltfParser::createImageWithCache(
    const tinygltf::Model &model,
    int imageIndex,
    std::map<int, osg::ref_ptr<osg::Image>> &imageCache)
{

    // Check cache first
    auto cacheIt = imageCache.find(imageIndex);
    if (cacheIt != imageCache.end())
    {
        return cacheIt->second;
    }

    // Create new image
    osg::ref_ptr<osg::Image> image = createImageFromGltf(model, imageIndex, imageCache);

    // Cache the image
    if (image.valid())
    {
        imageCache[imageIndex] = image;
    }

    return image;
}

void GltfParser::processMetallicRoughnessTexture(
    const tinygltf::Model &model,
    int textureIndex,
    osg::StateSet *stateSet,
    int textureUnit,
    std::map<int, osg::ref_ptr<osg::Texture2D>> &textureCache)
{

    auto [isValid, errorMsg] = validateTexture(model, textureIndex);
    if (!isValid)
    {
        std::cerr << "GltfParser: Metallic roughness texture validation failed: " << errorMsg << std::endl;
        return;
    }

    std::map<int, osg::ref_ptr<osg::Image>> imageCache;
    osg::ref_ptr<osg::Texture2D> texture = createTextureFromGltf(model, textureIndex, textureCache, imageCache);
    if (texture.valid())
    {
        stateSet->setTextureAttributeAndModes(textureUnit, texture, osg::StateAttribute::ON);

        // Metallic roughness texture typically stores:
        // R channel: Unused (or occlusion)
        // G channel: Roughness
        // B channel: Metallic
        // A channel: Unused
        std::cout << "GltfParser: Applied metallic roughness texture to texture unit " << textureUnit
                  << " (Note: requires shader support for proper channel parsing)" << std::endl;

        // In real applications, shader uniforms would be set here to tell the shader
        // how to parse the texture channels. Due to OSG fixed function pipeline limitations,
        // this can only be processed as a regular texture here.
    }
}

void GltfParser::processNormalTexture(
    const tinygltf::Model &model,
    const tinygltf::NormalTextureInfo &normalTexture,
    osg::StateSet *stateSet,
    int textureUnit,
    std::map<int, osg::ref_ptr<osg::Texture2D>> &textureCache)
{

    auto [isValid, errorMsg] = validateTexture(model, normalTexture.index);
    if (!isValid)
    {
        std::cerr << "GltfParser: Normal texture validation failed: " << errorMsg << std::endl;
        return;
    }

    std::map<int, osg::ref_ptr<osg::Image>> imageCache;
    osg::ref_ptr<osg::Texture2D> texture = createTextureFromGltf(model, normalTexture.index, textureCache, imageCache);
    if (texture.valid())
    {
        stateSet->setTextureAttributeAndModes(textureUnit, texture, osg::StateAttribute::ON);

        // Normal map intensity
        float scale = static_cast<float>(normalTexture.scale);
        std::cout << "GltfParser: Applied normal map to texture unit " << textureUnit
                  << ", intensity: " << scale
                  << " (Note: requires normal mapping shader support)" << std::endl;

        // In real applications, this would require:
        // 1. Using a normal mapping shader
        // 2. Passing the normal map intensity parameter
        // 3. Ensuring geometry has correct tangent space information
    }
}

void GltfParser::processOcclusionTexture(
    const tinygltf::Model &model,
    const tinygltf::OcclusionTextureInfo &occlusionTexture,
    osg::StateSet *stateSet,
    int textureUnit,
    std::map<int, osg::ref_ptr<osg::Texture2D>> &textureCache)
{

    auto [isValid, errorMsg] = validateTexture(model, occlusionTexture.index);
    if (!isValid)
    {
        std::cerr << "GltfParser: Occlusion texture validation failed: " << errorMsg << std::endl;
        return;
    }

    std::map<int, osg::ref_ptr<osg::Image>> imageCache;
    osg::ref_ptr<osg::Texture2D> texture = createTextureFromGltf(model, occlusionTexture.index, textureCache, imageCache);
    if (texture.valid())
    {
        stateSet->setTextureAttributeAndModes(textureUnit, texture, osg::StateAttribute::ON);

        // Occlusion intensity
        float strength = static_cast<float>(occlusionTexture.strength);
        std::cout << "GltfParser: Applied occlusion texture to texture unit " << textureUnit
                  << ", strength: " << strength
                  << " (Note: requires shader support for ambient occlusion)" << std::endl;

        // Occlusion texture typically stores data in R channel for ambient occlusion effects
    }
}

std::pair<bool, std::string> GltfParser::validateTexture(const tinygltf::Model &model, int textureIndex)
{
    if (textureIndex < 0 || textureIndex >= static_cast<int>(model.textures.size()))
    {
        return {false, "Texture index " + std::to_string(textureIndex) +
                           " out of range [0, " + std::to_string(model.textures.size()) + ")"};
    }

    const tinygltf::Texture &texture = model.textures[textureIndex];

    // Validate image index
    if (texture.source < 0 || texture.source >= static_cast<int>(model.images.size()))
    {
        return {false, "Texture " + std::to_string(textureIndex) +
                           " image index " + std::to_string(texture.source) +
                           " out of range [0, " + std::to_string(model.images.size()) + ")"};
    }

    // Validate sampler index (optional)
    if (texture.sampler >= 0 && texture.sampler >= static_cast<int>(model.samplers.size()))
    {
        return {false, "Texture " + std::to_string(textureIndex) +
                           " sampler index " + std::to_string(texture.sampler) +
                           " out of range [0, " + std::to_string(model.samplers.size()) + ")"};
    }

    // Validate image data
    const tinygltf::Image &image = model.images[texture.source];
    if (image.image.empty() && image.uri.empty() && image.bufferView < 0)
    {
        return {false, "Texture " + std::to_string(textureIndex) +
                           " image " + std::to_string(texture.source) + " has no valid data source"};
    }

    // Validate image dimensions
    if (!image.image.empty() && (image.width <= 0 || image.height <= 0))
    {
        return {false, "Texture " + std::to_string(textureIndex) +
                           " image " + std::to_string(texture.source) +
                           " invalid dimensions: " + std::to_string(image.width) + "x" + std::to_string(image.height)};
    }

    return {true, ""};
}
// Error handling methods implementation
void GltfParser::validateFileAccess(const std::string &filePath)
{
    if (filePath.empty())
    {
        throw GltfParseException(GltfError(GltfErrorType::FILE_NOT_FOUND, "File path is empty", filePath));
    }

    std::filesystem::path path(filePath);
    if (!std::filesystem::exists(path))
    {
        throw GltfParseException(GltfError(GltfErrorType::FILE_NOT_FOUND, "File does not exist", filePath));
    }

    if (!std::filesystem::is_regular_file(path))
    {
        throw GltfParseException(GltfError(GltfErrorType::FILE_ACCESS_ERROR, "Path is not a regular file", filePath));
    }

    // Check file size
    auto fileSize = std::filesystem::file_size(path);
    if (fileSize == 0)
    {
        throw GltfParseException(GltfError(GltfErrorType::INVALID_FORMAT, "File is empty", filePath));
    }

    // Check minimum file size (GLTF should be at least a few bytes)
    if (fileSize < 10)
    {
        throw GltfParseException(GltfError(GltfErrorType::INVALID_FORMAT, "File too small to be valid GLTF format", filePath));
    }
}

void GltfParser::validateModel(const tinygltf::Model &model, const std::string &fileName)
{
    // Validate basic model structure
    if (model.scenes.empty())
    {
        throw GltfParseException(GltfError(GltfErrorType::CORRUPTED_DATA, "Model has no scenes", fileName));
    }

    // Validate default scene index
    if (model.defaultScene >= static_cast<int>(model.scenes.size()))
    {
        PluginLogger::logWarning("GLTF", "Invalid default scene index: " + std::to_string(model.defaultScene) +
                                             ", using scene 0 instead");
    }

    // Validate nodes
    for (size_t i = 0; i < model.nodes.size(); ++i)
    {
        const tinygltf::Node &node = model.nodes[i];

        // Validate child node indices
        for (int childIndex : node.children)
        {
            if (childIndex < 0 || childIndex >= static_cast<int>(model.nodes.size()))
            {
                std::ostringstream oss;
                oss << "Invalid child node index " << childIndex << " in node " << i;
                throw GltfParseException(GltfError(GltfErrorType::INVALID_NODE_INDEX, oss.str(), fileName, i));
            }
        }

        // Validate mesh index
        if (node.mesh >= 0 && node.mesh >= static_cast<int>(model.meshes.size()))
        {
            std::ostringstream oss;
            oss << "Invalid mesh index " << node.mesh << " in node " << i;
            throw GltfParseException(GltfError(GltfErrorType::INVALID_MESH_INDEX, oss.str(), fileName, i));
        }

        // Validate transformation matrix/TRS values
        if (!node.matrix.empty() && node.matrix.size() != 16)
        {
            std::ostringstream oss;
            oss << "Invalid matrix size in node " << i << ": expected 16, got " << node.matrix.size();
            throw GltfParseException(GltfError(GltfErrorType::CORRUPTED_DATA, oss.str(), fileName, i));
        }

        // Check for finite values in transformation
        for (double value : node.matrix)
        {
            if (!std::isfinite(value))
            {
                std::ostringstream oss;
                oss << "Invalid transformation matrix value in node " << i;
                throw GltfParseException(GltfError(GltfErrorType::CORRUPTED_DATA, oss.str(), fileName, i));
            }
        }

        for (double value : node.translation)
        {
            if (!std::isfinite(value))
            {
                std::ostringstream oss;
                oss << "Invalid translation value in node " << i;
                throw GltfParseException(GltfError(GltfErrorType::CORRUPTED_DATA, oss.str(), fileName, i));
            }
        }

        for (double value : node.rotation)
        {
            if (!std::isfinite(value))
            {
                std::ostringstream oss;
                oss << "Invalid rotation value in node " << i;
                throw GltfParseException(GltfError(GltfErrorType::CORRUPTED_DATA, oss.str(), fileName, i));
            }
        }

        for (double value : node.scale)
        {
            if (!std::isfinite(value))
            {
                std::ostringstream oss;
                oss << "Invalid scale value in node " << i;
                throw GltfParseException(GltfError(GltfErrorType::CORRUPTED_DATA, oss.str(), fileName, i));
            }
        }
    }

    // Validate meshes
    for (size_t i = 0; i < model.meshes.size(); ++i)
    {
        const tinygltf::Mesh &mesh = model.meshes[i];

        if (mesh.primitives.empty())
        {
            PluginLogger::logWarning("GLTF", "Mesh " + std::to_string(i) + " has no primitives");
            continue;
        }

        for (size_t j = 0; j < mesh.primitives.size(); ++j)
        {
            const tinygltf::Primitive &primitive = mesh.primitives[j];

            // Validate material index
            if (primitive.material >= 0 && primitive.material >= static_cast<int>(model.materials.size()))
            {
                std::ostringstream oss;
                oss << "Invalid material index " << primitive.material << " in mesh " << i << " primitive " << j;
                throw GltfParseException(GltfError(GltfErrorType::INVALID_MATERIAL_INDEX, oss.str(), fileName, i));
            }

            // Validate accessor indices
            for (const auto &attr : primitive.attributes)
            {
                if (attr.second < 0 || attr.second >= static_cast<int>(model.accessors.size()))
                {
                    std::ostringstream oss;
                    oss << "Invalid accessor index " << attr.second << " for attribute " << attr.first
                        << " in mesh " << i << " primitive " << j;
                    throw GltfParseException(GltfError(GltfErrorType::INVALID_ACCESSOR_INDEX, oss.str(), fileName, i));
                }
            }

            // Validate indices accessor
            if (primitive.indices >= 0 && primitive.indices >= static_cast<int>(model.accessors.size()))
            {
                std::ostringstream oss;
                oss << "Invalid indices accessor index " << primitive.indices << " in mesh " << i << " primitive " << j;
                throw GltfParseException(GltfError(GltfErrorType::INVALID_ACCESSOR_INDEX, oss.str(), fileName, i));
            }
        }
    }

    // Validate materials
    for (size_t i = 0; i < model.materials.size(); ++i)
    {
        const tinygltf::Material &material = model.materials[i];

        // Validate texture indices
        if (material.pbrMetallicRoughness.baseColorTexture.index >= 0 &&
            material.pbrMetallicRoughness.baseColorTexture.index >= static_cast<int>(model.textures.size()))
        {
            std::ostringstream oss;
            oss << "Invalid base color texture index " << material.pbrMetallicRoughness.baseColorTexture.index
                << " in material " << i;
            throw GltfParseException(GltfError(GltfErrorType::INVALID_TEXTURE_INDEX, oss.str(), fileName, i));
        }

        if (material.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0 &&
            material.pbrMetallicRoughness.metallicRoughnessTexture.index >= static_cast<int>(model.textures.size()))
        {
            std::ostringstream oss;
            oss << "Invalid metallic roughness texture index " << material.pbrMetallicRoughness.metallicRoughnessTexture.index
                << " in material " << i;
            throw GltfParseException(GltfError(GltfErrorType::INVALID_TEXTURE_INDEX, oss.str(), fileName, i));
        }

        if (material.normalTexture.index >= 0 &&
            material.normalTexture.index >= static_cast<int>(model.textures.size()))
        {
            std::ostringstream oss;
            oss << "Invalid normal texture index " << material.normalTexture.index << " in material " << i;
            throw GltfParseException(GltfError(GltfErrorType::INVALID_TEXTURE_INDEX, oss.str(), fileName, i));
        }

        if (material.occlusionTexture.index >= 0 &&
            material.occlusionTexture.index >= static_cast<int>(model.textures.size()))
        {
            std::ostringstream oss;
            oss << "Invalid occlusion texture index " << material.occlusionTexture.index << " in material " << i;
            throw GltfParseException(GltfError(GltfErrorType::INVALID_TEXTURE_INDEX, oss.str(), fileName, i));
        }

        if (material.emissiveTexture.index >= 0 &&
            material.emissiveTexture.index >= static_cast<int>(model.textures.size()))
        {
            std::ostringstream oss;
            oss << "Invalid emissive texture index " << material.emissiveTexture.index << " in material " << i;
            throw GltfParseException(GltfError(GltfErrorType::INVALID_TEXTURE_INDEX, oss.str(), fileName, i));
        }
    }

    // Validate textures
    for (size_t i = 0; i < model.textures.size(); ++i)
    {
        const tinygltf::Texture &texture = model.textures[i];

        if (texture.source < 0 || texture.source >= static_cast<int>(model.images.size()))
        {
            std::ostringstream oss;
            oss << "Invalid image index " << texture.source << " in texture " << i;
            throw GltfParseException(GltfError(GltfErrorType::INVALID_IMAGE_INDEX, oss.str(), fileName, i));
        }

        if (texture.sampler >= 0 && texture.sampler >= static_cast<int>(model.samplers.size()))
        {
            std::ostringstream oss;
            oss << "Invalid sampler index " << texture.sampler << " in texture " << i;
            PluginLogger::logWarning("GLTF", oss.str());
        }
    }

    // Validate accessors
    for (size_t i = 0; i < model.accessors.size(); ++i)
    {
        const tinygltf::Accessor &accessor = model.accessors[i];

        if (accessor.bufferView >= 0 && accessor.bufferView >= static_cast<int>(model.bufferViews.size()))
        {
            std::ostringstream oss;
            oss << "Invalid buffer view index " << accessor.bufferView << " in accessor " << i;
            throw GltfParseException(GltfError(GltfErrorType::INVALID_BUFFER_VIEW_INDEX, oss.str(), fileName, i));
        }

        if (accessor.count == 0)
        {
            std::ostringstream oss;
            oss << "Accessor " << i << " has zero count";
            throw GltfParseException(GltfError(GltfErrorType::CORRUPTED_DATA, oss.str(), fileName, i));
        }
    }

    // Validate buffer views
    for (size_t i = 0; i < model.bufferViews.size(); ++i)
    {
        const tinygltf::BufferView &bufferView = model.bufferViews[i];

        if (bufferView.buffer < 0 || bufferView.buffer >= static_cast<int>(model.buffers.size()))
        {
            std::ostringstream oss;
            oss << "Invalid buffer index " << bufferView.buffer << " in buffer view " << i;
            throw GltfParseException(GltfError(GltfErrorType::CORRUPTED_DATA, oss.str(), fileName, i));
        }

        if (bufferView.byteLength == 0)
        {
            std::ostringstream oss;
            oss << "Buffer view " << i << " has zero byte length";
            throw GltfParseException(GltfError(GltfErrorType::CORRUPTED_DATA, oss.str(), fileName, i));
        }

        // Validate buffer bounds
        const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];
        if (bufferView.byteOffset + bufferView.byteLength > buffer.data.size())
        {
            std::ostringstream oss;
            oss << "Buffer view " << i << " exceeds buffer bounds";
            throw GltfParseException(GltfError(GltfErrorType::CORRUPTED_DATA, oss.str(), fileName, i));
        }
    }
}

void GltfParser::validateSceneIndex(const tinygltf::Model &model, int sceneIndex)
{
    if (sceneIndex < 0 || sceneIndex >= static_cast<int>(model.scenes.size()))
    {
        std::ostringstream oss;
        oss << "Scene index " << sceneIndex << " out of range [0, " << model.scenes.size() << ")";
        throw GltfParseException(GltfError(GltfErrorType::INVALID_SCENE_INDEX, oss.str(), "", sceneIndex));
    }
}

void GltfParser::validateNodeIndex(const tinygltf::Model &model, int nodeIndex)
{
    if (nodeIndex < 0 || nodeIndex >= static_cast<int>(model.nodes.size()))
    {
        std::ostringstream oss;
        oss << "Node index " << nodeIndex << " out of range [0, " << model.nodes.size() << ")";
        throw GltfParseException(GltfError(GltfErrorType::INVALID_NODE_INDEX, oss.str(), "", nodeIndex));
    }
}

void GltfParser::validateMeshIndex(const tinygltf::Model &model, int meshIndex)
{
    if (meshIndex < 0 || meshIndex >= static_cast<int>(model.meshes.size()))
    {
        std::ostringstream oss;
        oss << "Mesh index " << meshIndex << " out of range [0, " << model.meshes.size() << ")";
        throw GltfParseException(GltfError(GltfErrorType::INVALID_MESH_INDEX, oss.str(), "", meshIndex));
    }
}

void GltfParser::validateMaterialIndex(const tinygltf::Model &model, int materialIndex)
{
    if (materialIndex < 0 || materialIndex >= static_cast<int>(model.materials.size()))
    {
        std::ostringstream oss;
        oss << "Material index " << materialIndex << " out of range [0, " << model.materials.size() << ")";
        throw GltfParseException(GltfError(GltfErrorType::INVALID_MATERIAL_INDEX, oss.str(), "", materialIndex));
    }
}

void GltfParser::validateAccessorIndex(const tinygltf::Model &model, int accessorIndex)
{
    if (accessorIndex < 0 || accessorIndex >= static_cast<int>(model.accessors.size()))
    {
        std::ostringstream oss;
        oss << "Accessor index " << accessorIndex << " out of range [0, " << model.accessors.size() << ")";
        throw GltfParseException(GltfError(GltfErrorType::INVALID_ACCESSOR_INDEX, oss.str(), "", accessorIndex));
    }

    const tinygltf::Accessor &accessor = model.accessors[accessorIndex];

    // Validate accessor data
    if (accessor.bufferView >= 0)
    {
        if (accessor.bufferView >= static_cast<int>(model.bufferViews.size()))
        {
            std::ostringstream oss;
            oss << "Accessor " << accessorIndex << " references invalid buffer view " << accessor.bufferView;
            throw GltfParseException(GltfError(GltfErrorType::INVALID_BUFFER_VIEW_INDEX, oss.str(), "", accessorIndex));
        }

        const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
        if (bufferView.buffer >= static_cast<int>(model.buffers.size()))
        {
            std::ostringstream oss;
            oss << "Buffer view " << accessor.bufferView << " references invalid buffer " << bufferView.buffer;
            throw GltfParseException(GltfError(GltfErrorType::CORRUPTED_DATA, oss.str(), "", accessorIndex));
        }
    }
}

std::string GltfParser::getErrorTypeString(GltfErrorType type)
{
    switch (type)
    {
    case GltfErrorType::FILE_NOT_FOUND:
        return "FILE_NOT_FOUND";
    case GltfErrorType::FILE_ACCESS_ERROR:
        return "FILE_ACCESS_ERROR";
    case GltfErrorType::INVALID_FORMAT:
        return "INVALID_FORMAT";
    case GltfErrorType::UNSUPPORTED_FORMAT:
        return "UNSUPPORTED_FORMAT";
    case GltfErrorType::CORRUPTED_DATA:
        return "CORRUPTED_DATA";
    case GltfErrorType::INVALID_SCENE_INDEX:
        return "INVALID_SCENE_INDEX";
    case GltfErrorType::INVALID_NODE_INDEX:
        return "INVALID_NODE_INDEX";
    case GltfErrorType::INVALID_MESH_INDEX:
        return "INVALID_MESH_INDEX";
    case GltfErrorType::INVALID_MATERIAL_INDEX:
        return "INVALID_MATERIAL_INDEX";
    case GltfErrorType::INVALID_TEXTURE_INDEX:
        return "INVALID_TEXTURE_INDEX";
    case GltfErrorType::INVALID_IMAGE_INDEX:
        return "INVALID_IMAGE_INDEX";
    case GltfErrorType::INVALID_ACCESSOR_INDEX:
        return "INVALID_ACCESSOR_INDEX";
    case GltfErrorType::INVALID_BUFFER_VIEW_INDEX:
        return "INVALID_BUFFER_VIEW_INDEX";
    case GltfErrorType::MEMORY_ERROR:
        return "MEMORY_ERROR";
    case GltfErrorType::TINYGLTF_ERROR:
        return "TINYGLTF_ERROR";
    case GltfErrorType::UNKNOWN_ERROR:
        return "UNKNOWN_ERROR";
    default:
        return "UNKNOWN_ERROR_TYPE";
    }
}

void GltfParser::logError(GltfErrorType type, const std::string &message,
                          const std::string &fileName, int elementIndex)
{
    std::ostringstream oss;
    oss << getErrorTypeString(type) << ": " << message;
    if (!fileName.empty())
    {
        oss << " (file: " << fileName << ")";
    }
    if (elementIndex >= 0)
    {
        oss << " (element: " << elementIndex << ")";
    }
    PluginLogger::logError("GLTF", oss.str());
}