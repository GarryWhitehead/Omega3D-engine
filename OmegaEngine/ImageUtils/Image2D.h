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
class ImageCube2D : public Image2D
{

public:
	
private:

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
	*  Use this if you don't wnat to move a image across but need to manipulate in someway.
	* You need to ensure that the image isn;t destroyed whilst still associated with this class.
	* @param image Pointer to an already allocated portioin of memory where the image is held
	* @param width The width of the image in pixels
	* @param height The height of the image in pixels
	* @param channels The number of channels the image contains (i.e. rgba = 4)
	*/
	void setData(pixelType* image, uint32_t width, uint32_t height, uint8_t channels, uint8_t mipCount = 1)
	{
		assert(image);
		data = image;
		this->width = width;
		this->height = height;
		this->channels = channels;
		this->mipCount = mipCount;
		this->size = width * height * channels * faces;
	}

	/**
	* Allocates a chunk of memory as sepcified by the dimensions of the image
	* @param width The width of the image in pixels
	* @param height The height of the image in pixels
	* @param channels The number of channels the image contains (i.e. rgba = 4)
	*/
	void reserve(uint32_t width, uint32_t height, uint8_t channels)
	{
		data = new pixelType[width * height * channels];
		assert(data);
		this->width = width;
		this->height = height;
		this->channels = channels;
		this->size = width * height * channels;
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
	*/
	pixelType getPixel(uint32_t x, uint32_t y, uint8_t channel)
	{
		assert(data);
		assert(x <= width);
		assert(y <= height);
		assert(channel <= channels);
		return data[(x * channels) + channel + (y * width)];
	}

	/**
	* Sets a pixel to the specified value at the coord point indicated. 
	* @param x The x coord in pixels
	* @param y The y coord in pixels
	* @param channel The channel to retrive (0 = red; 1 = green; 2 = blue; 3 = alpha)
	*/
	void setPixel(pixelType pixel, uint32_t x, uint32_t y, uint8_t channel)
	{
		assert(x <= width);
		assert(y <= height);
		assert(channel <= channels);
		data[(x * channels) + channel + (y * width)] = pixel;
	}

	/**
	* Sets a pixel to the specified value at the coord point indicated. 
	* @param x The x coord in pixels
	* @param y The y coord in pixels
	* @param channel The channel to set (0 = red; 1 = green; 2 = blue; 3 = alpha)
	*/
	void setPixel(pixelType pixel, uint32_t x, uint32_t y, uint8_t channel)
	{
		assert(x <= width);
		assert(y <= height);
		assert(channel <= channels);
		data[(x * channels) + channel + (y * width)] = pixel;
	}

	/**
	* Sets a 3 channel pixel to the specified value at the coord point indicated. 
	* @param x The x coord in pixels
	* @param y The y coord in pixels
	* @param pixels A rgb definition of the pixel values
	*/
	void setPixel(pixelType pixel, uint32_t x, uint32_t y, Colour3& pixels)
	{
		assert(x <= width);
		assert(y <= height);
		assert(channels == 3 || channels == 4);
		size_t pos = (x * channels) + (y * width);
		data[pos] = pixel.r;
		data[pos + 1] = pixel.g;
		data[pos + 2] = pixel.b;
	}

	/**
	* Sets a 4 channel pixel to the specified value at the coord point indicated. 
	* @param x The x coord in pixels
	* @param y The y coord in pixels
	* @param channel The channel to retrive (0 = red; 1 = green; 2 = blue; 3 = alpha)
	*/
	void setPixel(pixelType pixel, uint32_t x, uint32_t y, Colour4& pixels)
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
	size_t size = 0;
	uint8_t faces = 1;	//< set to 6 if a cube
};
}    // namespace OmegaEngine
