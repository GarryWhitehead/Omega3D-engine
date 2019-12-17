#pragma once

#include "OEMaths/OEMaths.h"

#include <cassert>
#include <cstdint>
#include <utility>

namespace OmegaEngine
{

template <typename pixelType>
class Image2D
{
public:
	using Colour2 = OEMaths::VecN<pixelType, 2>;
	using Colour3 = OEMaths::VecN<pixelType, 3>;
	using Colour4 = OEMaths::VecN<pixelType, 4>;

	Image2D(pixelType* image, uint32_t width, uint32_t height, uint8_t channels);

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
	void reserve(uint32_t width, uint32_t height, uint8_t channels)
	{
		this->width = width;
		this->height = height;
		this->channels = channels;
		this->size = width * height * channels;

		// allocate image memory
		data = new pixelType[size];
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
	pixelType getTexel(uint32_t x, uint32_t y, uint8_t channel)
	{
		assert(data);
		assert(x <= width);
		assert(y <= height);
		assert(channel <= channels);

		size_t pixelPos = (x * channels) + channel + (y * width);
		return data[pixelPos];
	}

	/**
    * Retrieves a rgb value from a specific area of the image.
    * @param x The x coord in pixels
    * @param y The y coord in pixels
    * @param The face which to extract data from. This must be a cube image to be used
    */
	Colour3 getTexel3(uint32_t x, uint32_t y)
	{
		assert(data);
		assert(x <= width);
		assert(y <= height);
		assert(channels == 3);

		size_t pixelPos = x * channels + y * width;
		return { data[pixelPos], data[pixelPos + 1], data[pixelPos + 2] };
	}

	/**
    * Retrieves a rgba value from a specific area of the image.
    * @param x The x coord in pixels
    * @param y The y coord in pixels
    * @param The face which to extract data from. This must be a cube image to be used
    */
	Colour4 getTexel4(uint32_t x, uint32_t y)
	{
		assert(data);
		assert(x <= width);
		assert(y <= height);
		assert(channels == 4);

		size_t pixelPos = x * channels + y * width;

		return { data[pixelPos], data[pixelPos + 1], data[pixelPos + 2], data[pixelPos + 3] };
	}

	/**
	* Sets a pixel to the specified value at the coord point indicated. 
	* @param x The x coord in pixels
	* @param y The y coord in pixels
	* @param channel The channel to retrive (0 = red; 1 = green; 2 = blue; 3 = alpha)
	*/
	void writeTexel(pixelType pixel, uint32_t x, uint32_t y, uint8_t channel)
	{
		assert(x <= width);
		assert(y <= height);
		assert(channel <= channels);

		size_t pixelPos = (x * channels) + channel + (y * width);
		assert(pixelPos < size);
		data[pixelPos] = pixel;
	}

	/**
	* Sets a 3 channel pixel to the specified value at the coord point indicated. 
	* @param x The x coord in pixels
	* @param y The y coord in pixels
	* @param pixels A rgb definition of the pixel values
	*/
	void writeTexel(uint32_t x, uint32_t y, Colour3& pixels)
	{
		assert(x <= width);
		assert(y <= height);
		assert(channels == 3 || channels == 4);

		size_t pixelPos = x * channels + y * width;
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
	void writeTexel(uint32_t x, uint32_t y, Colour4& pixels)
	{
		assert(x <= width);
		assert(y <= height);
		assert(channels == 4);
		size_t pos = (x * channels) + (y * width);
		data[pos] = pixels.r;
		data[pos + 1] = pixels.g;
		data[pos + 2] = pixels.b;
		data[pos + 3] = pixels.a;
	}

	/**
	* @brief Returns the dimensions of the image. Only use this if your expecting both the width and height
	* dimensions to be the same.
	*/
	uint32_t getDimensions() const
	{
		assert(width == height);
		return width;
	}

	uint32_t getWidth() const 
	{
		return width;
	}

	uint32_t getHeight() const
	{
		return height;
	}

protected:
	// the actual image data
	pixelType* data = nullptr;

	uint32_t width = 0;
	uint32_t height = 0;
	uint8_t channels = 0;

	// total size of image
	size_t size = 0;
};

// useful types
using Image2DU8 = Image2D<uint8_t>;
using Image2DU16 = Image2D<uint16_t>;
using Image2DF32 = Image2D<float>;


}    // namespace OmegaEngine
