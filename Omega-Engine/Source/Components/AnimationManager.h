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

#include "Components/ComponentManager.h"
#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_Quat.h"

#include <memory>
#include <vector>

#define MAX_NUM_JOINTS 128

namespace OmegaEngine
{
// forward declerations
class OEObject;
class TransformManager;
class OEEngine;
class AnimInstance;

class AnimationManager : public ComponentManager
{

public:
    // animation
    struct Sampler
    {
        enum class InterpolationType
        {
            Linear,
            Step,
            CubicSpline
        } interpolationType;

        std::vector<float> timeStamps;
        std::vector<OEMaths::vec4f> outputs;

        uint32_t indexFromTime(double time);
        float getPhase(double time);
    };

    struct Channel
    {
        enum class PathType
        {
            Translation,
            Rotation,
            Scale,
            CublicTranslation,
            CubicScale
        };

        PathType type;
        OEObject* object;
        uint32_t samplerIndex;
    };

    struct AnimationInfo
    {
        const char* name;
        float start = std::numeric_limits<float>::max();
        float end = std::numeric_limits<float>::min();
        std::vector<Sampler> samplers;
        std::vector<Channel> channels;
    };

    AnimationManager();
    ~AnimationManager();

    void addAnimation(std::unique_ptr<AnimInstance>& animation);
    void addAnimation(size_t channelIndex, size_t bufferIndex, OEObject& object);

    void update(double time, OEEngine& engine);

    uint32_t getBufferOffset() const
    {
        return static_cast<uint32_t>(animations.size());
    }

private:
    std::vector<AnimationInfo> animations;
};

} // namespace OmegaEngine
