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

#ifndef INDIRECT_LIGHTING_H
#define INDIRECT_LIGHTING_H

#include "omega-engine/Skybox.h"
#include "utility/Compiler.h"

namespace OmegaEngine
{

class OE_PUBLIC IndirectLighting
{
public:
    /**
     @brief The environment map which will be used to create the irradiance and specular maps. This
     must be set if not specifying directly the irradiance and specular maps to use through calls to
     **specularMap** and **irradianceMap**
     */
    void setEnvMap(Skybox* skybox);

    /**
     @brief If set, the indirect lighting reflections will be created using the specified map. Note:
     if this is set, then the environment map set by **setEnvMap** will be ignored.
     */
    void specularMap(MappedTexture* tex);

    /**
    @brief If set, the indirect lighting irradiance will be created using the specified map. Note:
    if this is set, then the environment map set by **setEnvMap** will be ignored.
    */
    void irradianceMap(MappedTexture* tex);

protected:
    IndirectLighting() = default;
    ~IndirectLighting() = default;
};

} // namespace OmegaEngine

#endif /* IndirectLighting_h */
