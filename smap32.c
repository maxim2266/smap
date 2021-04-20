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

#if __SIZEOF_SIZE_T__ != 4
#error "This is 32 bit code."
#endif

#include "smap.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

// hashing ----------------------------------------------------------------------------------------
// implementation based on https://github.com/wangyi-fudan/wyhash
static size_t hash_seed;

static __attribute__((constructor))
void make_hash_seed(void)
{
	hash_seed = getpid() ^ time(NULL);

	switch(hash_seed)
	{
		case 0x429dacddu:	// known bad seeds
		case 0xd637dbf3u:
			++hash_seed;
	}
}

static inline
size_t load32(const uint8_t* const p)
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

#define mix(x, y)	\
({	\
	uint64_t c = (x) ^ 0x53c5ca59u;	\
	c *= (y) ^ 0x74743c1bu;	\
	(x) = (uint32_t)c;	\
	(y) = (uint32_t)(c >> 32);	\
})

static inline
uint32_t calc_hash_s(const smap_key s, const uint32_t seed)
{
	size_t i = s.len;
	uint32_t x = seed, y = i;

	mix(x, y);

	const uint8_t* p = (const uint8_t*)s.ptr;

	for(; i > 8; i -= 8, p += 8)	// "i > 8", like in the original code
	{
		x ^= load32(p);
		y ^= load32(p + 4);
		mix(x, y);
	}

	if(i >= 4)
	{
		x ^= load32(p);
		y ^= load32(p + i - 4);
	}
	else if(i > 0)
		x ^= load24(p, i);

	mix(x, y);
	mix(x, y);

	return x ^ y;
}

// the hash function
static inline
uint32_t calc_hash(const smap_key key)
{
	return calc_hash_s(key, hash_seed);
}

// map entry --------------------------------------------------------------------------------------
typedef struct
{
	uint32_t len;
	void*    value;
	char     key[];
} smap_entry;

static
smap_entry* new_entry(const smap_key key)
{
	smap_entry* const p = malloc(sizeof(smap_entry) + key.len + 1);

	if(!p)
		return NULL;

	p->len = key.len;
	p->value = NULL;

	memcpy(p->key, key.ptr, key.len);
	p->key[key.len] = 0;

	return p;
}

static inline
int match_entry(const smap_entry* const entry, const smap_key key)
{
	return key.len == entry->len && memcmp(key.ptr, entry->key, key.len) == 0;
}

// map slot ---------------------------------------------------------------------------------------
struct _smap_slot
{
	smap_entry*	entry;
	uint32_t 	hash;
};

static inline
smap_slot* find_slot(const smap* const map, const smap_key key, const uint32_t hash)
{
	const uint32_t m = map->mask;
	smap_slot* const slots = map->slots;
	uint32_t i;

	for(i = hash & m; slots[i].entry; i = (i + 1) & m)
		if(slots[i].hash == hash && match_entry(slots[i].entry, key))
			break;

	return slots + i;
}

// helpers ----------------------------------------------------------------------------------------
#define MIN_MASK	0xf
#define MIN_CAP 	(MIN_MASK - MIN_MASK / 4)
#define MAX_MASK	(UINT32_MAX / sizeof(smap_slot))
#define MAX_CAP		(MAX_MASK - MAX_MASK / 4)

static __attribute__((noinline))
int grow(smap* const map)
{
	if(map->mask == MAX_MASK)
		return 1;

	smap new_map = { .count = map->count, .mask = 2 * map->mask + 1 };

	if(!(new_map.slots = calloc((size_t)new_map.mask + 1, sizeof(new_map.slots[0]))))
		return 1;

	const smap_slot* p = map->slots;

	for(size_t n = map->count; n > 0; ++p, --n)
	{
		while(!p->entry)
			++p;

		uint32_t j = p->hash & new_map.mask;

		while(new_map.slots[j].entry)
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
		slots[i] = (smap_slot){ 0 };

		uint32_t k;

		do
		{
			j = (j + 1) & m;

			if(!slots[j].entry)
				return;

			k = slots[j].hash & m;
		} while((i <= j) ? (i < k && k <= j) : (i < k || k <= j));

		slots[i] = slots[j];
	}
}

#if __SIZEOF_INT__ == 4
#define clz  __builtin_clz
#elif __SIZEOF_LONG__ == 4
#define clz  __builtin_clzl
#else
#error "Strange platform."
#endif

static inline
uint32_t calc_mask(const uint32_t cap)
{
	if(cap <= MIN_CAP)
		return MIN_MASK;
	else if(cap > MAX_CAP)
		return MAX_MASK;
	else
	{
		const uint32_t c = UINT32_MAX >> clz(cap);

		return (c - c / 4 < cap) ? (2 * c + 1) : c;
	}
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
	smap_slot* p = map->slots;

	for(uint32_t n = map->count; n > 0; ++p, --n)
	{
		while(!p->entry)
			++p;

		free_value(p->entry->value);
		free(p->entry);
	}

	free(map->slots);
}

void** smap_get(const smap* const map, const smap_key key)
{
	if(map->count == 0)
		return NULL;

	smap_entry* const p = find_slot(map, key, calc_hash(key))->entry;

	return p ? &p->value : NULL;
}

void** smap_set(smap* const map, const smap_key key)
{
	const uint32_t hash = calc_hash(key);
	smap_slot* p = find_slot(map, key, hash);

	if(!p->entry)
	{
		if(map->count >= smap_cap(map))
		{
			if(grow(map))
				return NULL;

			p = find_slot(map, key, hash);
		}

		if(!(p->entry = new_entry(key)))
			return NULL;

		p->hash = hash;
		++map->count;
	}

	return &p->entry->value;
}

void* smap_del(smap* const map, const smap_key key)
{
	if(map->count == 0)
		return NULL;

	smap_slot* const p = find_slot(map, key, calc_hash(key));

	if(!p->entry)
		return NULL;

	void* const value = p->entry->value;

	free(p->entry);
	delete_slot(map, p - map->slots);

	return value;
}

int smap_scan(smap* const map, const smap_scan_func fn, void* const param)
{
	smap_slot* p = map->slots;

	for(uint32_t n = map->count; n > 0; --n)
	{
		while(!p->entry)
			++p;

		const int ret = fn(param, smap_bytes(p->entry->key, p->entry->len), &p->entry->value);

		switch(ret)
		{
			case -1:
				free(p->entry);
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

	const smap_slot* p = map->slots;

	for(size_t n = map->count; n > 0; ++p, --n)
	{
		while(!p->entry)
			++p;

		uint32_t j = p->hash & new_map.mask;

		while(new_map.slots[j].entry)
			j = (j + 1) & new_map.mask;

		new_map.slots[j] = *p;
	}

	free(map->slots);
	*map = new_map;

	return 0;
}

// wrappers for testing ---------------------------------------------------------------------------
#ifdef SMAP_TEST
uint32_t _smap_calc_mask(const uint32_t cap)						{ return calc_mask(cap); }
uint32_t _smap_calc_hash_s(const smap_key key, const uint32_t seed)	{ return calc_hash_s(key, seed); }
uint32_t _smap_calc_hash(const smap_key key)						{ return calc_hash(key); }
#endif
