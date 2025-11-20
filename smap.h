/*
BSD 3-Clause License

Copyright (c) 2021,2024, Maxim Konakov
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

#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// max. key length
#ifndef SMAP_MAX_KEY_LEN
// Just a sufficiently large number; can be redefined externally
#define SMAP_MAX_KEY_LEN (256 * 1024 * 1024)
#endif

// smap slot type
struct smap_slot_struct;
typedef struct smap_slot_struct smap_slot;

// smap type
typedef struct
{
	smap_slot* slots;
	size_t count, cap, seed;
} smap;

#define smap_empty ((smap){0})

// map capacity
static inline
size_t smap_cap(const smap* const map)	{ return map->cap - map->cap / 4; }

// number of items
static inline
size_t smap_size(const smap* const map)	{ return map->count; }

// clear the map
smap* smap_clear(smap* const map, void (*free_value)(void*));

// parameter validation
#define SMAP_CHECK_PARAMS(map, key, len)	\
	do {	\
		if((len) > SMAP_MAX_KEY_LEN) return NULL;	\
		if(!((key) && (len))) {	(key) = ""; (len) = 0; }	\
	} while(0)

// get item
static inline
void** smap_get(const smap* const map, const void* key, size_t len)
{
	extern void** smap_impl_get(const smap* const map, const void* const key, const size_t len);

	if(map->count == 0)
		return NULL;

	SMAP_CHECK_PARAMS(map, key, len);

	return smap_impl_get(map, key, len);
}

// add item
static inline
void** smap_add(smap* const map, const void* key, size_t len)
{
	extern void** smap_impl_add(smap* const map, const void* const key, const size_t len);

	SMAP_CHECK_PARAMS(map, key, len);

	return smap_impl_add(map, key, len);
}

// delete item
static inline
void* smap_del(smap* const map, const void* key, size_t len)
{
	extern void* smap_impl_del(smap* const map, const void* const key, const size_t len);

	if(map->count == 0)
		return NULL;

	SMAP_CHECK_PARAMS(map, key, len);

	return smap_impl_del(map, key, len);
}

// resize the map to allocate space for the given number of keys
int smap_resize(smap* const map, size_t n);

// scan the map calling the given function on each key; the return value of the callback
// is treated as:
//   0:   proceed with the scan;
//   < 0: delete the current key and proceed
//   > 0: stop the scan and return the value.
// The map being scanned must not be modified.

typedef int (*smap_scan_func)(void* param, const void* key, size_t len, void** pval);

int smap_scan(smap* const map, const smap_scan_func fn, void* const param);

#ifdef __cplusplus
}
#endif
