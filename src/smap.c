#include "impl.h"

#include <string.h>
#include <stdlib.h>

// hash entry constructor
static
smap_entry* make_entry(const void* const key, const size_t len)
{
	smap_entry* const entry = malloc(sizeof(smap_entry) + len);

	if(entry)
	{
		entry->value = NULL;
		entry->len = len;
		memcpy(entry->key, key, len);
	}

	return entry;
}

// compare entry to the given key
static inline
int match_entry(const smap_entry* const entry,
				const void* const key,
				const size_t len)
{
	return len == entry->len && memcmp(entry->key, key, len) == 0;
}

// find matching or first empty slot
smap_slot* smap_impl_find_slot(const smap* const map,
							   const size_t hash,
							   const void* const key,
							   const size_t len)
{
	const size_t mask = map->cap - 1;
	smap_slot* const slots = map->slots;
	size_t i;

	for(i = hash & mask; slots[i].entry; i = (i + 1) & mask)
		if(hash == slots[i].hash && match_entry(slots[i].entry, key, len))
			break;

	return &slots[i];
}

// resize the map to the given capacity
int smap_impl_resize(smap* const map, const size_t cap)
{
	// allocate new slots array
	smap_slot* const slots = calloc(cap, sizeof(smap_slot));

	if(!slots)
		return 1;

	// copy keys
	const size_t mask = cap - 1;
	const smap_slot* p = map->slots;

	for(size_t n = map->count; n > 0; ++p, --n)
	{
		while(!p->entry)
			++p;

		size_t i = p->hash & mask;

		while(slots[i].entry)
			i = (i + 1) & mask;

		slots[i] = *p;
	}

	// update the map struct
	free(map->slots);

	map->slots = slots;
	map->cap = cap;

	return 0;
}

// [API] get item
void** smap_impl_get(const smap* const map, const void* const key, const size_t len)
{
	const size_t hash = smap_impl_calc_hash(key, len, map->seed);
	smap_entry* const entry = smap_impl_find_slot(map, hash, key, len)->entry;

	return entry ? &entry->value : NULL;
}

// [API] add the key, if not present
void** smap_impl_add(smap* const map, const void* const key, const size_t len)
{
	// check if the map is empty
	if(!map->slots)
	{
		// allocate
		void* const slots = calloc(BASE_CAP, sizeof(smap_slot));

		if(!slots)
			return NULL;

		*map = (smap)
		{
			.slots = slots,
			.cap = BASE_CAP,
			.seed = smap_impl_hash_seed()
		};
	}

	// get the slot
	const size_t hash = smap_impl_calc_hash(key, len, map->seed);
	smap_slot* slot = smap_impl_find_slot(map, hash, key, len);

	// check if the key is not there
	if(!slot->entry)
	{
		// check if the count is at the limit
		if(map->count == smap_cap(map))
		{
			if(smap_impl_resize(map, map->cap * 2))
				return NULL;

			// get the slot again
			slot = smap_impl_find_slot(map, hash, key, len);
		}

		// insert the new key
		if(!(slot->entry = make_entry(key, len)))
			return NULL;

		slot->hash = hash;
		++map->count;
	}

	return &slot->entry->value;
}

// [API] clear the map
smap* smap_clear(smap* const map, void (*free_value)(void*))
{
	const smap_slot* p = map->slots;

	if(free_value)
	{
		for(size_t n = map->count; n > 0; --n, ++p)
		{
			while(!p->entry)
				++p;

			free_value(p->entry->value);
			free(p->entry);
		}
	}
	else
	{
		for(size_t n = map->count; n > 0; --n, ++p)
		{
			while(!p->entry)
				++p;

			free(p->entry);
		}
	}

	free(map->slots);
	*map = smap_empty;

	return map;
}
