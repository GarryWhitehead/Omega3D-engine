#include "GeneralUtil.h"


namespace Util
{
	/* CRC-32C (iSCSI) polynomial in reversed bit order. */
#define POLY 0x82f63b78

/* CRC-32 (Ethernet, ZIP, etc.) polynomial in reversed bit order. */
/* #define POLY 0xedb88320 */
	// crc should be 0 to start.
	uint32_t crc32c(uint32_t crc, const unsigned char *buf, size_t len)
	{
		int k;

		crc = ~crc;
		while (len--) {
			crc ^= *buf++;
			for (k = 0; k < 8; k++)
				crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		}
		return ~crc;
	}

	uint32_t generateTypeId(const char* typeName)
	{
		unsigned int id = 0;
		uint8_t *idAsByteArray = reinterpret_cast<uint8_t*>(&id);
		
	}

	void* alloc_align(size_t alignment_size, size_t size)
	{
		void *data = nullptr;
		data = _aligned_malloc(size, alignment_size);
		assert(data);
		return data;
	}
}
