#include "impl.h"

#include <time.h>
#include <sys/types.h>
#include <unistd.h>

#if __SIZEOF_SIZE_T__ == 8

#include "../wyhash/wyhash.h"

// hash secret
static
uint64_t hash_secret[4];

static __attribute__((constructor))
void init_hash(void)
{
	make_secret(getpid(), hash_secret);
}

// the hash calculator
size_t smap_impl_calc_hash(const void* const key, const size_t len, const size_t seed)
{
	return wyhash(key, len, seed, hash_secret);
}

// seed generator
size_t smap_impl_hash_seed(void)
{
	size_t seed = time(NULL);

	switch(seed & 0x1fffffffu)
	{
		case 0x14cc886eu:
		case 0x1bf4ed84u:
			++seed;
	}

	return seed;
}

#elif __SIZEOF_SIZE_T__ == 4

#include "../wyhash/wyhash32.h"

// the hash calculator
size_t smap_impl_calc_hash(const void* const key, const size_t len, const size_t seed)
{
	return wyhash32(key, len, seed);
}

// seed generator
size_t smap_impl_hash_seed(void)
{
	unsigned seed = time(NULL);

	switch(seed)
	{
		case 0x429dacddu:
		case 0xd637dbf3u:
			++seed;
	}

	return seed;
}

#else
	#error "unsupported CPU architecture"
#endif	// __SIZEOF_SIZE_T__
