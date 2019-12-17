#pragma once

#include "utility/CString.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace OmegaEngine
{

class ImageLoader
{
public:
	
	ImageLoader() = default;
	~ImageLoader()
	{
		if (buffer)
		{
			stbi_image_free(buffer);
			buffer = nullptr;
		}
	}

	// non-copyable or moveable
	ImageLoader(const ImageLoader&) = delete;
	ImageLoader& operator=(const ImageLoader&) = delete;
	ImageLoader(ImageLoader&&) = delete;
	ImageLoader& operator=(ImageLoader&&) = delete;

	/**
	* loads a image file supported by stb (jpg, png, etc.) into a bufffer
	* @brief filename absolute path to desired image file
	* @brief the number of components per pixel - this defaults to 4 (rgba)
	* @return Whther the image was successfully mapped into the buffer
 	*/
    bool load(Util::String filename, int components = 4);

	/**
	* @brief Returns the image width in pixels
	*/
	size_t getWidth() const
	{
		return static_cast<size_t>(width);
	}

	/**
	* @brief Returns the image height in pixels
	*/
	size_t getHeight() const
	{
		return static_cast<size_t>(height);
	}

	/**
	* @brief Returns the number of components per pixel
	*/
	int getComponentCount() const
	{
		return components;
	}

	/**
	* @brief Returns a pointer to the image buffer.
	* The total size of the buffer is width * height * components
	*/
	unsigned char* getData()
	{
		return buffer;
	}

private:
	int width;
	int height;
	int components;
	unsigned char* buffer;
};

}    // namespace OmegaEngine
