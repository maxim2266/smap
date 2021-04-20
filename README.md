# smap: a hash table for C language.

[![License: BSD 3 Clause](https://img.shields.io/badge/License-BSD_3--Clause-yellow.svg)](https://opensource.org/licenses/BSD-3-Clause)

## About
The hash table maps strings to `void*` pointers.

Main features:
* Simple and minimalist API;
* Good performance with typical compiler settings and without tuning for a particular CPU type;
* Support for both 32- and 64-bit platforms;
* Reduced memory consumption on 64-bit platforms.

## Installation
None really, just include the files `smap.h` and `smap32.c` and/or `smap64.c` into your project.
Here `smap.h` is the header with all declarations, `smap32.c` is the implementation for 32-bit
platforms, and `smap64.c` is the implementation for 64-bit platforms. Alternatively, add this
project as a `git` submodule.

## Code Example
```c
// the map object
smap map;

// initialise the map to have initial capacity of at least 42 entries
smap_init(&map, 42);

// insert new key (or replace existing one)
*smap_set(&map, smap_lit("some key")) = pointer_to_object_of_type_T;

// check if the key exists
assert(smap_get(&map, smap_lit("some key"));

// retrieve address of the value added above
T** pp = (T**)smap_get(&map, smap_lit("some key"));

assert(pp && *pp == pointer_to_object_of_type_T);
// and the value can also be replaced: *pp = some_other_pointer;

// or in one line, because the key exists
assert(*smap_get(&map, smap_lit("some key")) == pointer_to_object_of_type_T);

// delete the key
T* value = smap_del(&map, smap_lit("some key"));

assert(value == pointer_to_object_of_type_T);
free(value);

// or in one line, because the key exists, and assuming the value is not NULL
free(smap_del(&map, smap_lit("some key")));

// release the map, with all values passed to free() function
smap_release(&map, free);
```
In this example all checks for memory allocation errors are omitted for brevity.

#### Implementation
This is essentially an implementation of the open addressing scheme with linear probing,
like described, for example, [here](https://en.wikipedia.org/wiki/Open_addressing). The 64-bit
version has some optimisations (see below) to reduce memory consumption, while the 32-bit version
is quite straight-forward. Both versions use a re-implementation of
[wyhash](https://github.com/wangyi-fudan/wyhash) as the hash function.

#### Limits of the 64-bit version
In order to reduce memory consumption I have put some limits on the 64-bit version of the map:
* _Max. key length is 4G bytes._ I am yet to see a realistic use-case requiring longer keys.
* _Max. capacity is 3G entries._ Let's do some boring maths: at 75% utilisation the 3G entries translate
into 4G slots (baskets) of 8 bytes each, making it 32G bytes for the slots array, plus 3G of entries,
each of (16 + key length) bytes, taking up another 144G bytes for 32-byte keys and not counting
the metadata associated with each of the 3G memory allocations. In total, that's 176G bytes of memory.
I think most computes simply don't have that much RAM installed.
* _6 bytes of actual address._ (Applies to the implementation only, not to the `void*` values stored
in the map). This is a tricky one. It is based on the observation that on most mainstream platforms
when compiled for default memory model and with default linker script, the memory allocator returns
pointers with only 6 bytes representing the actual address, leaving the upper two bytes essentially
unused. Allocating entries with 8 bytes alignment gives another 3 bits, so that one 8 byte value can
store a pointer _and_ 19 bits of hash, thus reducing the memory occupied by the slots array, also
improving cache locality. This may or may not work in your particular set-up, so please check before
using this software.

## API

#### Key
The purpose of the `smap_key` type is to provide a convenient adaptor for various string
representations, that can also be extended to accept other types, like C++ `std::string`.

```c
typedef struct
{
    const char* ptr;  // pointer to the first byte of the key
    size_t len;       // key length, in bytes
} smap_key;
```
An object of the type can refer to any range of bytes, possibly with null bytes, as the map does
not require true C-strings as keys. A few constructors are provided for convenience:

`smap_key smap_str(const char* const s)`<br>
Construct a key from the given C string.

`smap_key smap_bytes(const char* const s, const size_t n)`<br>
Construct a key from the given range of bytes.

`smap_lit(key)`<br>
A macro to construct a key from a string literal.

#### Map

`typedef struct { ... } smap;`<br>
Opaque type of the hash map. Should not be accessed or modified other than via the provided API.

`int smap_init(smap* const map, const uint32_t cap)`<br>
Initialises the map to the specified capacity. The required capacity can also be set to 0, in
which case some small initial capacity is allocated. Returns 0 on success, or a non-zero value
on memory allocation failure.

`void smap_release(smap* const map, void (*free_value)(void*))`<br>
Releases resources allocated for the map. The map object itself is _not_ free'd, but is left in
a state suitable for re-initialisation using `smap_init()`. The `free_value()` callback function
is called on each value in the map and is expected to deallocate resources associated with
the value, if any.

`int smap_compact(smap* const map)`<br>
Optimises memory consumption for sparse maps. Returns 0 on success, or a non-zero value on
memory allocation failure.

`void** smap_get(const smap* const map, const smap_key key)`<br>
Retrieves the address of the value associated with the given key. Returns NULL if the
key is not found. The returned address is valid for as long as the given key is in the map,
and the value at that address can be modified.

`void** smap_set(smap* const map, const smap_key key)`<br>
Retrieves the address of the value associated with the given key, inserting the key if not already
present. Initial value of a newly inserted key is NULL. The returned address is valid for as long
as the given key is in the map. Returns NULL on memory allocation failure.

`void*  smap_del(smap* const map, const smap_key key)`<br>
Removes the given key from the map. Returns the value associated with the key, or NULL if the
key is not found. _Note_: NULL return value does not necessarily mean the key was not in the map,
because the value itself may be NULL.

`uint32_t smap_cap(const smap* const map)`<br>
Returns the current capacity of the map.

`uint32_t smap_size(const smap* const map)`<br>
Returns the number of keys currently in the map.

`int smap_scan(smap* const map, const smap_scan_func fn, void* const param)`<br>
Iterate the map calling the given callback function on each entry. Opaque `param` is passed
over to the callback function, unchanged.

`typedef int (*smap_scan_func)(void* param, const smap_key key, void** pval)`<br>
The type of callback function for `smap_scan()`. Parameters:
* `param`: the opaque value given to `smap_scan()` function;
* `key`: the key; it is guaranteed to be null-terminated;
* `pval`: address of the value associated with the `key`.

The value returned from the function is treated as follows:
* `0`: continue the scan to the next entry;
* `-1`: delete the current entry and continue the scan. Before returning `-1` the function
should deallocate all resources associated with the value at `pval`;
* any other value: stop the scan and return the value.

## Status
Tested on Linux Mint 20.1 with gcc v9.3.0 and clang v11.0.0.
