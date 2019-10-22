#pragma once

#include <cassert>
#include <cstdint>
#include <utility>

namespace OmegaEngine
{

template <typename pixelType>
struct Colour3
{
	pixelType r;
	pixelType g;
	pixelType b;
};

template <typename pixelType>
struct Colour4
{
	pixelType r;
	pixelType g;
	pixelType b;
	pixelType a;
};

template <typename pixelType>
class Image2D
{
public:

	enum class ImageType
	{
		Texture2D,
		Cube2D
	};

	Image2D(pixelType* image, uint32_t width, uint32_t height, uint8_t channels);
	
	Image2D(ImageType type)
	{
		if (type == ImageType::Cube2D)
		{
			this->faces = 6;
		}
	}

	// move contructor
	Image2D(Image2D&& other) noexcept
	    : data(std::exchange(other.data, nullptr))
	    , width(other.width)
	    , height(other.height)
	    , channels(other.channels)
	    , size(other.size)
	{
	}

	// move assignment
	Image2D& operator=(Image2D&& other) noexcept
	{
		if (this != &other)
		{
			data = std::exchange(other.data, nullptr);
			this->width = other.width;
			this->height = other.height;
			this->channels = other.channels;
			this->size = other.size;
		}
		return *this;
	}

	/**
	* Allocates a chunk of memory as sepcified by the dimensions of the image
	* @param width The width of the image in pixels
	* @param height The height of the image in pixels
	* @param channels The number of channels the image contains (i.e. rgba = 4)
	*/
	void reserve(uint32_t width, uint32_t height, uint8_t channels, uint8_t mipCount)
	{
		this->width = width;
		this->height = height;
		this->channels = channels;
        this->mipLevels = mipCount;
        
        // accumulate size for each mip map
        size_t totalSize = 0;
        for (uint8_t i = 0; i < mipLevels; ++i)
        {
            uint32_t mipWidth = width >> i;
            uint32_t mipHeight = height >> i;
            totalSize += mipWidth * mipHeight * channels * faces;
        }
        this->size = totalSize;
        
        // allocate image memory
        data = new pixelType[totalSize];
        assert(data);
	}

	/**
	* Fills an image with the specified value. Note: you must call **reserve** before this function
	* @param fillValue The value to fill the memory with 
	*/
	void fill(pixelType fillValue)
	{
		assert(data);
		memset(data, fillValue, size);
	}

	/**
	* Retrieves a pixel value from a specific area of the image. 
	* @param x The x coord in pixels
	* @param y The y coord in pixels
	* @param channel The channel to retrive (0 = red; 1 = green; 2 = blue; 3 = alpha)
    * @param The face which to extract data from. This must be a cube image to be used
	*/
	pixelType getPixel(uint32_t x, uint32_t y, uint8_t channel, uint8_t face = 1)
	{
		assert(data);
		assert(x <= width);
		assert(y <= height);
		assert(channel <= channels);
        if (face > 1 && type != ImageType::Cube)
        {
            LOGGER_ERROR("Trying to use a Image2D class a cube type when it has been initialised as a normal texture");
            return {};
        }

        size_t startPos = width * height * channels * (face - 1);
        size_t pixelPos = ((x * channels) + channel + (y * width)) + startPos;
		return data[pixelPos];
	}
    
    /**
    * Retrieves a rgb value from a specific area of the image.
    * @param x The x coord in pixels
    * @param y The y coord in pixels
    * @param The face which to extract data from. This must be a cube image to be used
    */
    Colour3 getPixel(uint32_t x, uint32_t y, uint8_t face = 1)
    {
        assert(data);
        assert(x <= width);
        assert(y <= height);
        assert(channels == 3);
        
        if (face > 1 && type != ImageType::Cube)
        {
            LOGGER_ERROR("Trying to use a Image2D class a cube type when it has been initialised as a normal texture");
            return {};
        }
        
        size_t startPos = width * height * channels * (face - 1);
        size_t pixelPos = ((x * channels) + (y * width)) + startPos;
        
        return {data[pixelPos], data[pixelPos + 1], data[pixelPos + 2]};
    }
    
    /**
    * Retrieves a rgba value from a specific area of the image.
    * @param x The x coord in pixels
    * @param y The y coord in pixels
    * @param The face which to extract data from. This must be a cube image to be used
    */
    Colour4 getPixel(uint32_t x, uint32_t y, uint8_t face = 1)
    {
        assert(data);
        assert(x <= width);
        assert(y <= height);
        assert(channels == 3);
        
        if (face > 1 && type != ImageType::Cube)
        {
            LOGGER_ERROR("Trying to use a Image2D class a cube type when it has been initialised as a normal texture");
            return {};
        }
        
        size_t startPos = width * height * channels * (face - 1);
        size_t pixelPos = ((x * channels) + (y * width)) + startPos;
        
        return {data[pixelPos], data[pixelPos + 1], data[pixelPos + 2]};
    }

	/**
	* Sets a pixel to the specified value at the coord point indicated. 
	* @param x The x coord in pixels
	* @param y The y coord in pixels
	* @param channel The channel to retrive (0 = red; 1 = green; 2 = blue; 3 = alpha)
	*/
	void setPixel(pixelType pixel, uint32_t x, uint32_t y, uint8_t channel, uint8_t face = 1)
	{
		assert(x <= width);
		assert(y <= height);
		assert(channel <= channels);
        
        if (face > 1 && type != ImageType::Cube)
        {
            LOGGER_ERROR("Trying to use a Image2D class a cube type when it has been initialised as a normal texture");
            return {};
        }
        
        size_t startPos = width * height * channels * (face - 1);
        size_t pixelPos = ((x * channels) + channel + (y * width)) + startPos;
        assert(pixelPos < size);
		data[pixelPos] = pixel;
	}

	/**
	* Sets a 3 channel pixel to the specified value at the coord point indicated. 
	* @param x The x coord in pixels
	* @param y The y coord in pixels
	* @param pixels A rgb definition of the pixel values
	*/
	void setPixel(pixelType pixel, uint32_t x, uint32_t y, Colour3& pixels, uint8_t face = 1)
	{
		assert(x <= width);
		assert(y <= height);
		assert(channels == 3 || channels == 4);
		
        if (face > 1 && type != ImageType::Cube)
        {
            LOGGER_ERROR("Trying to use a Image2D class a cube type when it has been initialised as a normal texture");
            return {};
        }
        
        size_t startPos = width * height * channels * (face - 1);
        size_t pixelPos = ((x * channels) + (y * width)) + startPos;
        assert(pixelPos < size);
        
		data[pixelPos] = pixels.r;
		data[pixelPos + 1] = pixels.g;
		data[pixelPos + 2] = pixels.b;
	}

	/**
	* Sets a 4 channel pixel to the specified value at the coord point indicated. 
	* @param x The x coord in pixels
	* @param y The y coord in pixels
	* @param channel The channel to retrive (0 = red; 1 = green; 2 = blue; 3 = alpha)
	*/
	void setPixel(pixelType pixel, uint32_t x, uint32_t y, Colour4& pixels, uint8_t face = 1)
	{
		assert(x <= width);
		assert(y <= height);
		assert(channels == 4);
		size_t pos = (x * channels) + (y * width); 
		data[pos] = pixel.r;
		data[pos + 1] = pixel.g;
		data[pos + 2] = pixel.b;
		data[pos + 3] = pixel.a;
	}

protected:
	// the actual image data
	pixelType* data = nullptr;

	uint32_t width = 0;
	uint32_t height = 0;
	uint8_t channels = 0;
    uint8_t mipLevels = 1;
    uint8_t faces = 1;    //< set to 6 if a cube
	
    // total size of image including all faces and mip maps
    size_t size = 0;
	
};

// useful types
using Image2DU8 = Image2D<uint8_t>;
using Image2DU16 = Image2D<uint16_t>;
using Image2DF32 = Image2DL<float>;

}    // namespace OmegaEngine
