#ifndef HS_HASH_MAP_H
#define HS_HASH_MAP_H

#include <stddef.h>
#include <stdbool.h>

typedef bool (*hs_equal_func)(const void *first, const void *second);

typedef size_t (*hs_hash_func)(const void *data);

typedef void (*hs_unary_func)(void *data);

typedef void (*hs_iter_func)(void *key, void *value);

typedef void (*hs_const_iter_func)(const void *key, const void *value);

/**
 * Opaque data structure representing the map.
 * Should only be accessed using the functions declared below.
 */
typedef struct _hs_hash_map hs_hash_map;

/**
 * Creates new instance of hash map.
 *
 * @param hash_func Key hash function.
 * @param equal_func Function for testing keys for equality.
 * @return Pointer to created map.
 */
hs_hash_map *hs_hash_map_new(hs_hash_func hash_func, hs_equal_func equal_func);

/**
 * Creates new instance of hash map with additional parameters.
 *
 * @param hash_func Key hash function.
 * @param equal_func Function for testing keys for equality.
 * @param key_remove_notify Function called when key is removed from map.
 * @param value_remove_notify Function called when value is removed from map.
 * @return  Pointer to created map.
 */
hs_hash_map *hs_hash_map_new_extended(hs_hash_func hash_func,
                                      hs_equal_func equal_func,
                                      hs_unary_func key_remove_notify,
                                      hs_unary_func value_remove_notify);

/**
 * Adds the corresponding key-value pair to the map.
 * If the key already exists, overwrites the value.
 * Note that key can be NULL if provided hash and comparison
 * functions are NULL-safe.
 *
 * @param map Target map.
 * @param key Key pointer.
 * @param value Value pointer.
 * @return true on success, false otherwise.
 */
bool hs_hash_map_put(hs_hash_map *map, void *key, void *value);

/**
 * Retrieves value by key.
 * Note that if you use NULL values, it is impossible to distinguish them from
 * non-existent ones. Use hs_hash_map_has_key() for this purpose.
 *
 * @param map Target map.
 * @param key Key pointer.
 * @return Value pointer if key exists, NULL otherwise.
 */
void *hs_hash_map_get(hs_hash_map *map, const void *key);

/**
 * Const version of hs_hash_map_get().
 *
 * @param map Target map.
 * @param key Key pointer.
 * @return Value pointer if key exists, NULL otherwise.
 */
const void *hs_hash_map_get_const(const hs_hash_map *map, const void *key);

/**
 * Removes the key-value pair matching specified key (if it exists).
 *
 * @param map Target map.
 * @param key Key pointer.
 */
void hs_hash_map_remove(hs_hash_map *map, const void *key);

/**
 * Copies all the keys to specified location.
 *
 * @param map Target map.
 * @param dst location to copy keys to; must be at least
 *            sizeof(void *) * (number of elements in the map) bytes long.
 */
void hs_hash_map_get_keys(const hs_hash_map *map, void *dst[]);

/**
 * Copies all the values to specified location.
 *
 * @param map target map
 * @param dst location to copy values to; must be at least
 *            sizeof(void *) * (number of elements in the map) bytes long.
 */
void hs_hash_map_get_values(const hs_hash_map *map, void *dst[]);

/**
 * Copies all the key-value pairs to specified location.
 *
 * @param map Target map.
 * @param dst location to copy entries to; must be at least
 *            2 * sizeof(void *) * (number of elements in the map) bytes long.
 */
void hs_hash_map_get_entries(const hs_hash_map *map, void *dst[]);

/**
 * Returns number of entries contained in the map.
 *
 * @param map Target map.
 * @return Size of the map.
 */
size_t hs_hash_map_size(const hs_hash_map *map);

/**
 * Checks if the specified key is present within the map.
 *
 * @param map Target map.
 * @param key Key pointer.
 * @return true if key exists, false otherwise.
 */
bool hs_hash_map_has_key(const hs_hash_map *map, const void *key);

/**
 * Returns load factor of the map (ratio between size & total capacity).
 *
 * @param map Target map.
 * @return Map's load factor.
 */
double hs_hash_map_load_factor(const hs_hash_map *map);

/**
 * Checks whether the map contains any key-value pairs.
 *
 * @param map Target map.
 * @return true if map is empty, false otherwise.
 */
bool hs_hash_map_is_empty(const hs_hash_map *map);

/**
 * Iterates over all map's key-value pairs.
 *
 * @param map Target map.
 * @param iterator Pointer to iterator function.
 */
void hs_hash_map_for_each(hs_hash_map *map, hs_iter_func iterator);

/**
 * Const version of hs_hash_map_for_each().
 *
 * @param map Target map.
 * @param iterator Pointer to iterator function.
 */
void hs_hash_map_for_each_const(const hs_hash_map *map,
                                hs_const_iter_func iterator);

/**
 * Deallocates the memory occupied by the map.
 *
 * @param map Target map.
 */
void hs_hash_map_free(hs_hash_map *map);


#endif // HS_HASH_MAP_H
