#include "GeneralUtil.h"
#include <cstring>

namespace Util
{
// CRC-32C (iSCSI) polynomial in reversed bit order.
uint32_t crc32c(uint32_t crc, const char *buf, size_t len)
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

uint32_t generateTypeId(const char *typeName)
{
	return crc32c(0, typeName, std::strlen(typeName));
}

void *alloc_align(size_t alignmentSize, size_t size)
{
	void *data = nullptr;
	data = _aligned_malloc(size, alignmentSize);
	assert(data);
	return data;
}
} // namespace Util
