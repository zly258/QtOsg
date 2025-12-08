#ifndef READERWRITERGLTF_H
#define READERWRITERGLTF_H

#include <osgDB/ReaderWriter>
#include <osg/Group>
#include <osg/Node>
#include <string>

/**
 * @brief OSG plugin for reading GLTF/GLB files
 *
 * This plugin provides support for loading GLTF (ASCII) and GLB (binary) files
 * using the independent GltfParser implementation.
 */
class ReaderWriterGLTF : public osgDB::ReaderWriter
{
public:
    ReaderWriterGLTF();
    virtual ~ReaderWriterGLTF() = default;

    virtual const char *className() const override;

    virtual ReadResult readNode(const std::string &fileName, const osgDB::Options *options) const override;
    virtual ReadResult readNode(std::istream &stream, const osgDB::Options *options) const override;
    virtual WriteResult writeNode(const osg::Node &node, const std::string &fileName, const osgDB::Options *options) const override;
};

#endif // READERWRITERGLTF_H