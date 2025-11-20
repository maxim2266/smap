#include "impl.h"

#include <stdlib.h>

// [API] scan the map calling the given function on each key
int smap_scan(smap* const map, const smap_scan_func fn, void* const param)
{
	smap_slot* p = map->slots;

	for(size_t n = map->count; n > 0; --n)
	{
		while(!p->entry)
			++p;

		const int ret = fn(param, p->entry->key, p->entry->len, &p->entry->value);

		if(ret == 0)		// proceed to the next
			++p;
		else if(ret > 0)	// fail
			return ret;
		else				// delete current key and proceed
		{
			free(p->entry);
			smap_impl_delete_slot(map, p - map->slots);
		}
	}

	return 0;
}
