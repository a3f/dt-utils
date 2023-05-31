#include <stdint.h>
#include <assert.h>

#include <dt/common.h>

int main(void)
{
	const char *str = "Hello, World!";
	uint32_t checksum;

	checksum = crc32(0, str, strlen(str));
	assert(checksum == 0xec4ac3d0);

	checksum = crc32_no_comp(0, str, strlen(str));
	assert(checksum == 0xe33e8552);

	return 0;
}
