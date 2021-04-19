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

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// smap slot type
#if __SIZEOF_SIZE_T__ == 8

typedef uintptr_t smap_slot;

#elif __SIZEOF_SIZE_T__ == 4

struct _smap_slot;
typedef struct _smap_slot smap_slot;

#else
#error "Unsupported platform"
#endif

// smap key
typedef struct
{
	const char* ptr;
	size_t len;
} smap_key;

static inline
smap_key smap_str(const char* const s)
{
	return s ? ((smap_key){ s, strlen(s) }) : ((smap_key){ "", 0 });
}

static inline
smap_key smap_bytes(const char* const s, const size_t n)
{
	return (s && n) ? ((smap_key){ s, n }) : ((smap_key){ "", 0 });
}

#define smap_lit(key)	((smap_key){ "" key, sizeof(key) - 1 })
#define smap_bin(key)	((smap_key){ (const char*)&(key), sizeof(key) })	// requires lvalue key

// smap
typedef struct
{
	smap_slot* slots;
	uint32_t count, mask;
} smap;

int smap_init(smap* const map, const uint32_t cap);
int smap_compact(smap* const map);
void smap_release(smap* const map, void (*free_value)(void*));

void** smap_get(const smap* const map, const smap_key key) __attribute__((pure));
void** smap_set(smap* const map, const smap_key key);
void*  smap_del(smap* const map, const smap_key key);

static inline
uint32_t smap_cap(const smap* const map)	{ return map->mask - map->mask / 4; }

static inline
uint32_t smap_size(const smap* const map)	{ return map->count; }

// scan the map calling the given function on each key; return value of the function treated as:
//   0:  proceed with the scan;
//   -1: delete the current key and proceed; don't forget to delete the value before return!!
//   any other: stop the scan and return the value.
// The map being scanned must not be modified.

typedef int (*smap_scan_func)(void* param, const smap_key key, void** pval);

int smap_scan(smap* const map, const smap_scan_func fn, void* const param);

#ifdef __cplusplus
}
#endif
