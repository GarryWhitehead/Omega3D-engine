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

#ifndef SCENE_HPP
#define SCENE_HPP

#include "omega-engine/Skybox.h"
#include "utility/Compiler.h"

namespace OmegaEngine
{
class Engine;
class World;
class Camera;
class Skybox;
class IndirectLighting;

class OE_PUBLIC Scene
{
public:
    Scene() = default;

    // a scene isn't copyable
    Scene(Scene&) = delete;
    Scene& operator=(Scene&) = delete;

    void prepare();

    Camera* getCurrentCamera();

    bool addSkybox(Skybox* instance);

    void addCamera(Camera* camera);

    void addIndirectLighting(IndirectLighting* ibl);

private:
};

} // namespace OmegaEngine

#endif
