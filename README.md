# smap: a hash table for C language.

[![License: BSD 3 Clause](https://img.shields.io/badge/License-BSD_3--Clause-yellow.svg)](https://opensource.org/licenses/BSD-3-Clause)

### About
The hash table maps keys (byte arrays) to `void*` pointers.

_Main features:_
* Simple and minimalist API;
* Good [performance](bench64.md) with typical compiler settings and without tuning for a particular CPU type;
* Support for both 32- and 64-bit platforms;

### Installation
```bash
git clone --recurse-submodules 'https://github.com/maxim2266/smap'
cd smap
make   # or 'make test' to run tests as well
```

After that just `#include` the file `smap.h` into your source code, and link with `libsmap.a`.
Extra compiler options can be given to `make`, for example:

```bash
make 'CFLAGS=-flto'
```

It is usually convenient to include this project as a `git` submodule.

### Code Example
```c
// declare a map object and initialise it
smap map = smap_empty;

// insert a new key (or replace the existing one); all values in the map have
// type void*, and the function below returns a pointer to the location
// of the value (i.e., void**), so adding a key and a value can be done like
int key = ...
*smap_add(&map, &key, sizeof(key)) = pointer_to_object_of_type_T;

// check if the key exists
assert(smap_get(&map, &key, sizeof(key));

// retrieve address of the value added above
T** pp = (T**)smap_get(&map, &key, sizeof(key));

assert(pp && *pp == pointer_to_object_of_type_T);
// and the value can also be replaced: *pp = some_other_pointer;

// or all in one line, because the key exists
assert(*smap_get(&map, &key, sizeof(key)) == pointer_to_object_of_type_T);

// delete the key
T* value = smap_del(&map, &key, sizeof(key));

assert(value == pointer_to_object_of_type_T);
free(value);

// or all in one line, because the key exists
free(smap_del(&map, smap_lit("some key")));

// clear the map, with all values passed to free() function
smap_release(&map, free);
```
In this example all checks for memory allocation failures are omitted for brevity.

### Implementation
This is essentially an implementation of the open addressing scheme with linear probing,
like described, for example, [here](https://en.wikipedia.org/wiki/Open_addressing), and
[wyhash](https://github.com/wangyi-fudan/wyhash) is used as the hash function.

### API

`typedef struct { ... } smap;`<br>
Opaque type of the hash map. Should not be accessed or modified other than via the provided API.
Each new instance of the map _must_ be initialised with `smap_empty` value.

`smap* smap_clear(smap* const map, void (*free_value)(void*))`<br>
Releases resources allocated for the map. The map object itself is reset to empty state and can be
reused. The `free_value()` callback function (if not NULL) is called once per each value in the map
and is expected to deallocate resources associated with the value, if any. The return value is a copy
of the first parameter, just to simplify deallocation when the map itself is on the heap:
`free(smap_clear(map, free))`

`int smap_resize(smap* const map, size_t n)`<br>
Resizes the map to accommodate for the given number of keys, or the number of keys currently in
the map, whichever is greater. Returns 0 on success, or a non-zero value on memory allocation failure.

`void** smap_get(const smap* const map, const void* key, size_t len)`<br>
Retrieves the address of the value associated with the given key. Returns NULL if the
key is not found. The returned address is valid for as long as the given key is in the map,
and the value at that address can be modified.

`void** smap_add(smap* const map, const void* key, size_t len)`<br>
Retrieves the address of the value associated with the given key, inserting the key if not already
present. The map will grow as necessary. Initial value of a newly inserted key is NULL. The returned
address is valid for as long as the given key is in the map. Returns NULL on memory allocation failure.

`void* smap_del(smap* const map, const void* key, size_t len)`<br>
Removes the given key from the map. Returns the value associated with the key, or NULL if the
key is not found. _Note_: NULL return value does not necessarily mean the key was not in the map,
because the value itself may be NULL.

`uint32_t smap_cap(const smap* const map)`<br>
Returns the current capacity of the map.

`uint32_t smap_size(const smap* const map)`<br>
Returns the number of keys currently in the map.

`int smap_scan(smap* const map, const smap_scan_func fn, void* const param)`<br>
Iterate the map calling the given callback function on each entry. Opaque `param` is passed
over to the callback function, unmodified. The callback function has the type:<br>
`typedef int (*smap_scan_func)(void* param, const void* key, size_t len, void** pval)`<br>
and the parameters are:
* `param`: the opaque value given to `smap_scan()` function;
* `key`: pointer to the key;
* `len`: length of the key;
* `pval`: address of the value associated with the `key`.

The value returned from the function is treated as follows:
* `0`: proceed to the next entry;
* `< 0`: delete the current entry and continue. Before returning a negative value the function
should deallocate all resources associated with the value at `pval`;
* `> 0`: stop the scan and return the value.

### Status
Tested and benchmarked on Linux Mint 21.3 with gcc v11.4.0 and clang v14.0.0.
