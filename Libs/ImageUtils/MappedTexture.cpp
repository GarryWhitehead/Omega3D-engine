#include "MappedTexture.h"

#include "utility/Logger.h"

#include <algorithm>
#include <cassert>
#include <string>

namespace OmegaEngine
{

MappedTexture::~MappedTexture()
{
    if (buffer)
    {
        delete[] buffer;
    }
}

bool MappedTexture::mapTexture(
    uint8_t* data,
    uint32_t w,
    uint32_t h,
    uint32_t faces,
    uint32_t arrays,
    uint32_t mips,
    uint32_t size,
    const ImageFormat imageFormat)
{
    width = w;
    height = h;
    mipLevels = mips;
    faceCount = faces;
    arrayCount = arrays;
    totalSize = size;
    assert(totalSize);

    format = imageFormat;

    this->buffer = new uint8_t[totalSize];
    if (!this->buffer)
    {
        LOGGER_INFO("Error whilst allocationg memory for texture. Out of memory?");
        return false;
    }

    memcpy(this->buffer, data, totalSize);
    return true;
}

bool MappedTexture::load(Util::String filePath)
{
    // get the extension
    auto split = Util::String::split(filePath, '.');
    Util::String ext = split.back();

    // check whether a directory has been specified
    Util::String absPath = filePath;
    if (!dirPath.empty())
    {
        absPath = Util::String::append(dirPath, absPath);
    }

    if (split.size() != 2)
    {
        LOGGER_ERROR(
            "File name %s has no extension. Unable to interpret image format.", absPath.c_str());
        return false;
    }

    if (ext.compare("ktx"))
    {
        KtxReader parser;
        if (!parser.loadFile(absPath))
        {
            LOGGER_ERROR("Unable to open ktx image file: %s", absPath.c_str());
            return false;
        }

        KtxReader::ImageOutput* data = parser.getImageData();
        width = data->width;
        height = data->height;
        mipLevels = data->mipLevels;
        arrayCount = data->arrayCount;
        faceCount = data->faceCount;
        totalSize = data->totalSize;
        // TODO: need a better way of handling image formats here from ktx files. 
        format = ImageFormat::RGBA_Unorm;

        assert(totalSize);
        assert(data->data);

        // create the buffer and copy the pixels across
        buffer = new uint8_t[totalSize];
        memcpy(buffer, data->data, totalSize);
    }
    else if (ext.compare("png") || ext.compare("jpg"))
    {
        ImageLoader loader;
        if (!loader.load(absPath))
        {
            return false;
        }

        width = loader.getWidth();
        height = loader.getHeight();
        format = ImageFormat::RGBA_Unorm;

        // calculate total size
        int comp = loader.getComponentCount();

        // we only support rgba textures, so the textures is only rgb, create a new buffer of the
        // correct type
        if (comp == 3)
        {
            comp = 4;
            uint32_t oldSize = width * height * 3;
            uint32_t newSize = width * height * comp;
            buffer = new unsigned char[newSize];
            assert(buffer);

            // the alpha channel will be empty (memset here?)
            memcpy(buffer, loader.getData(), oldSize);
        }
        else
        {
            totalSize = width * height * comp;
            buffer = new uint8_t[totalSize];
            assert(buffer);
            memcpy(buffer, loader.getData(), totalSize);
        }
    }
    else
    {
        LOGGER_ERROR("Unsupported image extension. File path: %s", filePath.c_str());
        return false;
    }

    return true;
}

void MappedTexture::setDirectory(Util::String dir)
{
    dirPath = dir;
}

bool MappedTexture::isCubeMap() const
{
    return faceCount == 6;
}

uint8_t* MappedTexture::getBuffer()
{
    return buffer;
}

Util::String& MappedTexture::getName()
{
    return name;
}

uint32_t MappedTexture::getWidth() const
{
    return width;
}

uint32_t MappedTexture::getHeight() const
{
    return height;
}

ImageFormat MappedTexture::getFormat() const
{
    return format;
}

uint32_t MappedTexture::getMipLevelCount() const
{
    return mipLevels;
}

uint32_t MappedTexture::getFaceCount() const
{
    return faceCount;
}


} // namespace OmegaEngine
