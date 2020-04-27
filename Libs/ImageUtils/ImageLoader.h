/* Copyright (c) 2018-2020 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include "stb_image.h"
#include "utility/CString.h"

namespace OmegaEngine
{

class ImageLoader
{
public:
    ImageLoader() = default;
    ~ImageLoader();

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

} // namespace OmegaEngine
