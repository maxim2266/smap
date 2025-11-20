#include "impl.h"

#include <stdlib.h>

// delete slot at the given index
void smap_impl_delete_slot(smap* const map, size_t i)
{
	--map->count;

	const size_t mask = map->cap - 1;
	smap_slot* const slots = map->slots;

	for(size_t j = i; ; i = j)
	{
		slots[i] = (smap_slot){0};

		size_t k;

		do
		{
			j = (j + 1) & mask;

			if(!slots[j].entry)
				return;

			k = slots[j].hash & mask;
		} while((i <= j) ? (i < k && k <= j) : (i < k || k <= j));

		slots[i] = slots[j];
	}
}

// [API] delete item
void* smap_impl_del(smap* const map, const void* const key, const size_t len)
{
	const size_t hash = smap_impl_calc_hash(key, len, map->seed);
	smap_slot* const slot = smap_impl_find_slot(map, hash, key, len);

	if(!slot->entry)
		return NULL;

	void* const value = slot->entry->value;

	free(slot->entry);
	smap_impl_delete_slot(map, slot - map->slots);

	return value;
}

