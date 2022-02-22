/*
BSD 3-Clause License

Copyright (c) 2021, Maxim Konakov
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#if __SIZEOF_SIZE_T__ != 8
#error "This is 64 bit code."
#endif

#include "smap.h"

#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

// hashing ----------------------------------------------------------------------------------------
// implementation based on https://github.com/wangyi-fudan/wyhash
static uint64_t hash_seed;

static __attribute__((constructor))
void make_hash_seed(void)
{
	hash_seed = getpid() ^ time(NULL);

	switch(hash_seed & 0x1fffffffu)
	{
		case 0x14cc886eu:
		case 0x1bf4ed84u:
			++hash_seed;
	}

	hash_seed ^= 0xa0761d6478bd642full;	// moved here from the hash function
}

static inline
size_t load_uint32(const uint8_t* const p)
{
	uint32_t v;

	memcpy(&v, p, sizeof(v));
	return v;
}

static inline
size_t load24(const uint8_t* const p, const size_t k)
{
	return (((size_t)p[0]) << 16) | (((size_t)p[k >> 1]) << 8) | p[k - 1];
}

static inline
uint64_t load_uint64(const uint8_t* const p)
{
	uint64_t v;

	memcpy(&v, p, sizeof(v));
	return v;
}

#if defined(_MSC_VER) && defined(_M_X64)
#include <intrin.h>
#pragma intrinsic(_umul128)
#endif

static inline
uint64_t mix(uint64_t x, uint64_t y)
{
#ifdef __SIZEOF_INT128__

	__uint128_t r = x;

	r *= y;
	x = (uint64_t)r;
	y = (uint64_t)(r >> 64);

#elif defined(_MSC_VER) && defined(_M_X64)

	x = _umul128(x, y, &y);

#else

	const uint64_t
		ha = (x) >> 32,
		hb = (y) >> 32,
		la = (uint32_t)x,
		lb = (uint32_t)y,
		rh = ha * hb,
		rm0 = ha * lb,
		rm1 = hb * la,
		rl = la * lb,
		t = rl + (rm0 << 32),
		c = t < rl;

	x = t + (rm1 << 32);
	y = rh + (rm0 >> 32) + (rm1 >> 32) + (c + (x < t));

#endif

	return x ^ y;
}

static inline
uint64_t calc_hash_s(const smap_key key, uint64_t seed)
{
	const uint64_t
		s1 = 0xe7037ed1a0b428dbull,
		s2 = 0x8ebc6af09c88c6e3ull,
		s3 = 0x589965cc75374cc3ull;

	uint64_t a, b;
	const uint8_t* p = (const uint8_t*)key.ptr;

	if(key.len <= 16)
	{
		if(key.len >= 4)
		{
			a = (load_uint32(p) << 32) | load_uint32(p + ((key.len >> 3) << 2));
			b = (load_uint32(p + key.len - 4) << 32) | load_uint32(p + key.len - 4 - ((key.len >> 3) << 2));
		}
		else if(key.len > 0)
		{
			a = load24(p, key.len);
			b = 0;
		}
		else
			a = b = 0;
	}
	else
	{
		size_t i = key.len;

		if(i > 48)
		{
			uint64_t x = seed, y = seed;

			do {
				seed = mix(load_uint64(p) ^ s1, load_uint64(p + 8) ^ seed);
				x = mix(load_uint64(p + 16) ^ s2, load_uint64(p + 24) ^ x);
				y = mix(load_uint64(p + 32) ^ s3, load_uint64(p + 40) ^ y);
				p += 48;
				i -= 48;
			} while(i > 48);

			seed ^= x ^ y;
		}

		for(; i > 16; i -= 16, p += 16)
			seed = mix(load_uint64(p) ^ s1, load_uint64(p + 8) ^ seed);

		a = load_uint64(p + i - 16);
		b = load_uint64(p + i - 8);
	}

	return mix(s1 ^ key.len, mix(a ^ s1, b ^ seed));
}

// the hash function
static inline
uint32_t calc_hash(const smap_key key)
{
	return (uint32_t)calc_hash_s(key, hash_seed);
}

// hash entry -------------------------------------------------------------------------------------
typedef struct
{
	void* value;
	uint32_t hash, len;
	char key[];
} smap_entry;

#define ENTRY_ALIGN	(sizeof(void*))

static
smap_entry* new_entry(const smap_key key, const uint32_t hash)
{
	if(key.len >= (size_t)UINT32_MAX)
		return NULL;

	const size_t n = sizeof(smap_entry) + key.len + 1;
	smap_entry* const entry = aligned_alloc(ENTRY_ALIGN, (n / ENTRY_ALIGN + 1) * ENTRY_ALIGN);

	if(!entry)
		return NULL;

	entry->value = NULL;
	entry->hash = hash;
	entry->len = key.len;

	memcpy(entry->key, key.ptr, key.len);
	entry->key[key.len] = 0;

	return entry;
}

#undef ENTRY_ALIGN

static inline
int match_entry(const smap_entry* const entry, const smap_key key)
{
	return key.len == entry->len && memcmp(key.ptr, entry->key, key.len) == 0;
}

// hash slot --------------------------------------------------------------------------------------
#define HASH_MASK	0x7ffffu	// 19 bits

static inline
smap_entry* entry_ptr(const uintptr_t slot)
{
	return (smap_entry*)((slot & ~(uintptr_t)HASH_MASK) >> 16);
}

static inline
uintptr_t make_slot(smap_entry* const entry, const uint32_t hash)
{
	return ((uintptr_t)entry << 16) | (hash & HASH_MASK);
}

static inline
uintptr_t* find_slot(const smap* const map, const smap_key key, const uint32_t hash)
{
	const uint32_t m = map->mask;
	uintptr_t* const slots = map->slots;
	uint32_t i;

	for(i = hash & m; slots[i]; i = (i + 1) & m)
		if(((hash ^ slots[i]) & HASH_MASK) == 0 && match_entry(entry_ptr(slots[i]), key))
			break;

	return slots + i;
}

// helpers ----------------------------------------------------------------------------------------
static __attribute__((noinline))
int grow(smap* const map)
{
	if(map->mask == UINT32_MAX)
		return 1;

	smap new_map = { .count = map->count, .mask = 2 * map->mask + 1 };

	if(!(new_map.slots = calloc((size_t)new_map.mask + 1, sizeof(new_map.slots[0]))))
		return 1;

	const uintptr_t* p = map->slots;

	for(size_t n = map->count; n > 0; ++p, --n)
	{
		while(*p == (uintptr_t)0)
			++p;

		uint32_t j = ((new_map.mask > HASH_MASK) ? entry_ptr(*p)->hash : *p) & new_map.mask;

		while(new_map.slots[j] != (uintptr_t)0)
			j = (j + 1) & new_map.mask;

		new_map.slots[j] = *p;
	}

	free(map->slots);
	*map = new_map;

	return 0;
}

static
void delete_slot(smap* const map, uint32_t i)
{
	--map->count;

	const uint32_t m = map->mask;
	smap_slot* const slots = map->slots;

	for(uint32_t j = i; ; i = j)
	{
		slots[i] = (uintptr_t)0;

		uint32_t k;

		do
		{
			j = (j + 1) & m;

			if(slots[j] == (uintptr_t)0)
				return;

			k = ((m > HASH_MASK) ? entry_ptr(slots[j])->hash : slots[j]) & m;
		} while((i <= j) ? (i < k && k <= j) : (i < k || k <= j));

		slots[i] = slots[j];
	}
}

#if __SIZEOF_INT__ == 4
#define clz  __builtin_clz
#elif __SIZEOF_LONG__ == 4
#define clz  __builtin_clzl
#else
#error "Unsupported platform."
#endif

#define MIN_MASK	0xf
#define MIN_CAP 	(MIN_MASK - MIN_MASK / 4)

static inline
uint32_t calc_mask(const uint32_t cap)
{
	if(cap <= MIN_CAP)
		return MIN_MASK;

	if(cap > UINT32_MAX - UINT32_MAX / 4)
		return UINT32_MAX;

	const uint32_t c = UINT32_MAX >> clz(cap);

	return (c - c / 4 < cap) ? ((c << 1) | 1) : c;
}

// API implementation -----------------------------------------------------------------------------
int smap_init(smap* const map, const uint32_t cap)
{
	map->count = 0;
	map->mask = calc_mask(cap);
	map->slots = calloc((size_t)map->mask + 1, sizeof(map->slots[0]));

	return map->slots == NULL;
}

void smap_release(smap* const map, void (*free_value)(void*))
{
	uintptr_t* p = map->slots;

	for(uint32_t n = map->count; n > 0; ++p, --n)
	{
		while(*p == (uintptr_t)0)
			++p;

		smap_entry* entry = entry_ptr(*p);

		free_value(entry->value);
		free(entry);
	}

	free(map->slots);
}

void** smap_get(const smap* const map, const smap_key key)
{
	if(map->count == 0)
		return NULL;

	const uintptr_t slot = *find_slot(map, key,  calc_hash(key));

	return slot ? &entry_ptr(slot)->value : NULL;
}

void** smap_set(smap* const map, const smap_key key)
{
	const uint32_t hash = calc_hash(key);
	uintptr_t* p = find_slot(map, key, hash);
	smap_entry* entry = entry_ptr(*p);

	if(!entry)
	{
		if(map->count >= smap_cap(map))
		{
			if(grow(map))
				return NULL;

			p = find_slot(map, key, hash);
		}

		entry = new_entry(key, hash);

		if(!entry)
			return NULL;

		*p = make_slot(entry, hash);
		++map->count;
	}

	return &entry->value;
}

void* smap_del(smap* const map, const smap_key key)
{
	if(map->count == 0)
		return NULL;

	uintptr_t* const p = find_slot(map, key, calc_hash(key));
	smap_entry* const entry = entry_ptr(*p);

	if(!entry)
		return NULL;

	void* const value = entry->value;

	free(entry);
	delete_slot(map, p - map->slots);

	return value;
}

int smap_scan(smap* const map, const smap_scan_func fn, void* const param)
{
	uintptr_t* p = map->slots;

	for(uint32_t n = map->count; n > 0; --n)
	{
		while(*p == (uintptr_t)0)
			++p;

		smap_entry* const entry = entry_ptr(*p);
		const int ret = fn(param, smap_bytes(entry->key, entry->len), &entry->value);

		switch(ret)
		{
			case -1:
				free(entry);
				delete_slot(map, p - map->slots);
				break;
			case 0:
				++p;
				break;
			default:
				return ret;
		}
	}

	return 0;
}

int smap_compact(smap* const map)
{
	if(map->mask == MIN_MASK || map->count > smap_cap(map) / 8)
		return 0;

	smap new_map;

	if(smap_init(&new_map, map->count))
		return 1;

	new_map.count = map->count;

	const uintptr_t* p = map->slots;

	for(size_t n = map->count; n > 0; ++p, --n)
	{
		while(*p == (uintptr_t)0)
			++p;

		uint32_t j = ((new_map.mask > HASH_MASK) ? entry_ptr(*p)->hash : *p) & new_map.mask;

		while(new_map.slots[j] != (uintptr_t)0)
			j = (j + 1) & new_map.mask;

		new_map.slots[j] = *p;
	}

	free(map->slots);
	*map = new_map;

	return 0;
}

// wrappers for testing ---------------------------------------------------------------------------
#ifdef SMAP_TEST
uint32_t _smap_calc_mask(const uint32_t cap)					{ return calc_mask(cap); }
uint64_t _smap_calc_hash_s(const smap_key key, uint64_t seed)	{ return calc_hash_s(key, seed); }
uint32_t _smap_calc_hash(const smap_key key)					{ return calc_hash(key); }
#endif	// #ifdef SMAP_TEST
