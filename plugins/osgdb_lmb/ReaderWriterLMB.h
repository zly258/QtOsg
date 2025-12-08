#ifndef READERWRITERLMB_H
#define READERWRITERLMB_H

#include <osgDB/ReaderWriter>
#include <osg/Group>
#include <osg/Node>
#include <string>

/**
 * @brief OSG plugin for reading LMB files
 *
 * This plugin provides support for loading LMB files
 * using the independent LmbParser implementation.
 */
class ReaderWriterLMB : public osgDB::ReaderWriter
{
public:
    ReaderWriterLMB();
    virtual ~ReaderWriterLMB() = default;

    virtual const char *className() const override;

    virtual ReadResult readNode(const std::string &fileName, const osgDB::Options *options) const override;
    virtual ReadResult readNode(std::istream &stream, const osgDB::Options *options) const override;
    virtual WriteResult writeNode(const osg::Node &node, const std::string &fileName, const osgDB::Options *options) const override;
};

#endif // READERWRITERLMB_H