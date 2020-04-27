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

#include "Object.h"

namespace OmegaEngine
{

OEObject::OEObject(const uint64_t id) : id(id)
{
}

// operator overloads
bool OEObject::operator==(const OEObject& obj) const
{
    return id == obj.id;
}

// helper functions
uint64_t OEObject::getId() const
{
    return id;
}

void OEObject::setId(const uint64_t objId)
{
    id = objId;
}

bool OEObject::isActive() const
{
    return active;
}

// ================= front-end ===========================================

uint64_t Object::getId() const
{
    return static_cast<const OEObject*>(this)->getId();
}

bool Object::isActive() const
{
    return static_cast<const OEObject*>(this)->isActive();
}

} // namespace OmegaEngine
