#include "impl.h"

// round up to the next power of 2
static
size_t round_up(size_t x)
{
	// from "Hacker's Delight" by Henry S. Warren, Jr.
	--x;

	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;

	return x + 1;
}

// [API] resize the map to allocate space for the given number of keys
int smap_resize(smap* const map, size_t n)
{
	// check the value of n
	if(map->count > n)
		n = map->count;

	if(n == 0)
	{
		smap_clear(map, NULL);
		return 0;
	}

	size_t cap = round_up(n);	// capacity must be a power of 2

	if(cap < BASE_CAP)
		cap = BASE_CAP;

	if(n >= cap - cap / 4)
		cap *= 2;

	return (cap == map->cap) ? 0 : _smap_resize(map, cap);
}

