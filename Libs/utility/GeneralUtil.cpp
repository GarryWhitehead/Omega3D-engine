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

#include "GeneralUtil.h"

#include <cstring>

namespace Util
{
// CRC-32C (iSCSI) polynomial in reversed bit order.
uint32_t crc32c(uint32_t crc, const char* buf, size_t len)
{
    const uint32_t POLY = 0x82f63b78;

    // crc should be zero to start.
    crc = ~crc;

    while (len--)
    {
        crc ^= *buf++;
        for (uint32_t k = 0; k < 8; k++)
        {
            crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
        }
    }
    return ~crc;
}

uint32_t generateTypeId(const char* typeName)
{
    return crc32c(0, typeName, std::strlen(typeName));
}
} // namespace Util
