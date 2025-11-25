#pragma once

#include "../smap.h"

#include <stdint.h>

// hash functions
size_t smap_impl_hash_seed(void);

size_t smap_impl_calc_hash(const void* const key, const size_t len, const size_t seed);

// smallest capacity of a non-empty smap
#define BASE_CAP 16

// hash entry
typedef struct
{
	void*   value;	// user-provided value
	size_t  len;	// key length
	uint8_t key[];	// key bytes
} smap_entry;

// hash slot
struct smap_slot_struct
{
	smap_entry* entry;
	size_t      hash;
};

// find slot for the given key and hash
smap_slot* smap_impl_find_slot(const smap* const map,
							   const size_t hash,
							   const void* const key,
							   const size_t len);

// delete slot at the given index
void smap_impl_delete_slot(smap* const map, size_t i);

// resize the map to the given capacity
int smap_impl_resize(smap* const map, const size_t cap);
