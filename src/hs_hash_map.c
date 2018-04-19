#include <hs_hash_map/hs_hash_map.h>

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define HS_HASH_MAP_INITIAL_CAPACITY 32
#define HS_HASH_MAP_VIRTUAL_BUCKET_SIZE 32

typedef uint32_t hs_bitmap;

typedef struct {
        hs_bitmap hop_info;
        bool has_value;
        void *key;
        void *value;
} hs_hash_map_bucket;

struct _hs_hash_map {
        hs_hash_func hash_func;
        hs_equal_func equal_func;
        hs_unary_func key_remove_notify;
        hs_unary_func value_remove_notify;
        size_t size;
        size_t capacity;
        hs_hash_map_bucket *buckets;
};

static inline void hs_hash_map_set_bit(hs_bitmap *bitmap, unsigned position)
{
        *bitmap |= (1 << (HS_HASH_MAP_VIRTUAL_BUCKET_SIZE - 1 - position));
}

static inline void hs_hash_map_clear_bit(hs_bitmap *bitmap, unsigned position)
{
        *bitmap &= ~((uint32_t) (1 << (HS_HASH_MAP_VIRTUAL_BUCKET_SIZE -
                                       1 - position)));
}

static bool hs_hash_map_get_bit(const hs_bitmap *bitmap, unsigned position)
{
        return (*bitmap &
                (1 << (HS_HASH_MAP_VIRTUAL_BUCKET_SIZE - 1 - position))) != 0;
}

static inline void hs_hash_map_put_to_bucket(hs_hash_map *map,
                                             hs_hash_map_bucket *bucket,
                                             void *key, void *value)
{
        bucket->key = key;
        bucket->value = value;
        bucket->has_value = true;
        ++map->size;
}

static void hs_hash_map_swap_bucket_contents(hs_hash_map_bucket *first,
                                             hs_hash_map_bucket *second)
{
        void *temp_content;
        bool temp_value;
        temp_content = first->key;
        first->key = second->key;
        second->key = temp_content;
        temp_content = first->value;
        first->value = second->value;
        second->value = temp_content;
        temp_value = first->has_value;
        first->has_value = second->has_value;
        second->has_value = temp_value;
}

static hs_hash_map_bucket *hs_hash_map_find_bucket_extended(
        const hs_hash_map *map,
        const void *key,
        size_t index,
        size_t *initial_index,
        size_t *index_offset)
{
        hs_hash_map_bucket *start_bucket = map->buckets + index;
        hs_hash_map_bucket *bucket = NULL;
        size_t offset = 0;
        do {
                while (offset < HS_HASH_MAP_VIRTUAL_BUCKET_SIZE &&
                       !hs_hash_map_get_bit(&start_bucket->hop_info,
                                            (unsigned) offset))
                        ++offset;
                if (offset == HS_HASH_MAP_VIRTUAL_BUCKET_SIZE)
                        return NULL;
                bucket = map->buckets + index + offset++;
        } while (!map->equal_func(bucket->key, key));
        if (initial_index)
                *initial_index = index;
        if (index_offset)
                *index_offset = offset - 1;
        return bucket;
}

static hs_hash_map_bucket *hs_hash_map_find_bucket(const hs_hash_map *map,
                                                   const void *key)
{
        return hs_hash_map_find_bucket_extended(map, key,
                                                map->hash_func(key) %
                                                map->capacity, NULL, NULL);
}

static bool hs_hash_map_put_internal(hs_hash_map *map, void *key, void *value)
{
        size_t start_index = map->hash_func(key) % map->capacity;
        hs_hash_map_bucket *bucket =
                hs_hash_map_find_bucket_extended(map, key, start_index, NULL,
                                                 NULL);
        if (bucket) {
                void *old_value = bucket->value;
                bucket->value = value;
                if (map->value_remove_notify)
                        map->value_remove_notify(old_value);
                return true;
        }

        size_t index = start_index;
        hs_hash_map_bucket *start_bucket = bucket = map->buckets + index;
        while (bucket->has_value && index < (map->capacity - 1))
                bucket = map->buckets + (++index);
        if (bucket->has_value)
                return false;
        hs_hash_map_bucket *empty_bucket = bucket;
        size_t empty_index = index;
        while (empty_index - start_index >= HS_HASH_MAP_VIRTUAL_BUCKET_SIZE) {
                index = empty_index + 1 - HS_HASH_MAP_VIRTUAL_BUCKET_SIZE;
                while (index < empty_index) {
                        bucket = map->buckets + index;
                        size_t initial = map->hash_func(bucket->key) %
                                         map->capacity;
                        if (empty_index - initial >=
                            HS_HASH_MAP_VIRTUAL_BUCKET_SIZE) {
                                ++index;
                                continue;
                        }
                        hs_hash_map_swap_bucket_contents(bucket, empty_bucket);
                        hs_hash_map_bucket *initial_bucket =
                                map->buckets + initial;
                        hs_hash_map_clear_bit(&initial_bucket->hop_info,
                                              (unsigned) (index - initial));
                        hs_hash_map_set_bit(&initial_bucket->hop_info,
                                            (unsigned) (empty_index - initial));
                        empty_index = index++;
                        empty_bucket = bucket;
                        break;
                }
                // No suitable empty buckets were found in the neighbourhood of
                // the target bucket
                if (index == empty_index)
                        return false;
        }
        hs_hash_map_set_bit(&start_bucket->hop_info,
                            (unsigned) (empty_index - start_index));
        hs_hash_map_put_to_bucket(map, empty_bucket, key, value);
        return true;
}

void *hs_hash_map_get_internal(const hs_hash_map *map, const void *key)
{
        hs_hash_map_bucket *bucket = hs_hash_map_find_bucket(map, key);
        if (!bucket)
                return NULL;
        return bucket->value;
}

static bool hs_hash_map_rehash(hs_hash_map *map)
{
        // Triggered when collision is encountered during rehash
        bool bad_rehash;
        hs_hash_map *temp = malloc(sizeof(hs_hash_map));
        if (!temp)
                return false;
        temp->size = 0;
        temp->hash_func = map->hash_func;
        temp->equal_func = map->equal_func;
        temp->key_remove_notify = NULL;
        temp->value_remove_notify = NULL;
        temp->capacity = map->capacity;
        temp->buckets = NULL;
        do {
                bad_rehash = false;
                free(temp->buckets);
                temp->capacity = temp->capacity * 2;
                temp->buckets = calloc(temp->capacity,
                                       sizeof(hs_hash_map_bucket));
                if (!temp->buckets) {
                        free(temp);
                        return false;
                }
                for (size_t i = 0; (i < map->capacity) && !bad_rehash; ++i) {
                        hs_hash_map_bucket *bucket = map->buckets + i;
                        if (!bucket->has_value)
                                continue;
                        bad_rehash = !hs_hash_map_put_internal(temp,
                                                               bucket->key,
                                                               bucket->value);
                }
        } while (bad_rehash);
        map->capacity = temp->capacity;
        free(map->buckets);
        map->buckets = temp->buckets;
        free(temp);
        return true;
}

hs_hash_map *hs_hash_map_new(hs_hash_func hash_func, hs_equal_func equal_func)
{
        return hs_hash_map_new_extended(hash_func, equal_func, NULL, NULL);
}

hs_hash_map *hs_hash_map_new_extended(hs_hash_func hash_func,
                                      hs_equal_func equal_func,
                                      hs_unary_func key_remove_notify,
                                      hs_unary_func value_remove_notify)
{
        hs_hash_map *map = malloc(sizeof(hs_hash_map));
        if (!map)
                return NULL;
        map->hash_func = hash_func;
        map->equal_func = equal_func;
        map->key_remove_notify = key_remove_notify;
        map->value_remove_notify = value_remove_notify;
        map->size = 0;
        map->capacity = HS_HASH_MAP_INITIAL_CAPACITY;
        map->buckets = calloc(map->capacity, sizeof(hs_hash_map_bucket));
        if (!map->buckets) {
                free(map);
                return NULL;
        }
        return map;
}

bool hs_hash_map_put(hs_hash_map *map, void *key, void *value)
{
        while (!hs_hash_map_put_internal(map, key, value)) {
                if (!hs_hash_map_rehash(map))
                        return false;
        }
        return true;

}

void *hs_hash_map_get(hs_hash_map *map, const void *key)
{
        return hs_hash_map_get_internal(map, key);
}

const void *hs_hash_map_get_const(const hs_hash_map *map, const void *key)
{
        return hs_hash_map_get_internal(map, key);
}

void hs_hash_map_remove(hs_hash_map *map, const void *key)
{
        size_t index, offset;
        hs_hash_map_bucket *bucket =
                hs_hash_map_find_bucket_extended(map, key, map->hash_func(key) %
                                                           map->capacity,
                                                 &index, &offset);
        if (bucket) {
                hs_hash_map_bucket *initial = map->buckets + index;
                hs_hash_map_clear_bit(&initial->hop_info, (unsigned) offset);
                bucket->has_value = false;
                --map->size;
                if (map->key_remove_notify)
                        map->key_remove_notify(bucket->key);
                if (map->value_remove_notify)
                        map->value_remove_notify(bucket->value);
        }
}

void hs_hash_map_get_keys(const hs_hash_map *map, void *dst[])
{
        size_t i = 0;
        for (size_t j = 0; j < map->capacity; ++j) {
                hs_hash_map_bucket *bucket = map->buckets + j;
                if (bucket->has_value)
                        dst[i++] = bucket->key;
        }
}

void hs_hash_map_get_values(const hs_hash_map *map, void *dst[])
{
        size_t i = 0;
        for (size_t j = 0; j < map->capacity; ++j) {
                hs_hash_map_bucket *bucket = map->buckets + j;
                if (bucket->has_value)
                        dst[i++] = bucket->value;
        }
}

void hs_hash_map_get_entries(const hs_hash_map *map, void *dst[])
{
        size_t i = 0;
        for (size_t j = 0; j < map->capacity; ++j) {
                hs_hash_map_bucket *bucket = map->buckets + j;
                if (bucket->has_value) {
                        dst[i++] = bucket->key;
                        dst[i++] = bucket->value;
                }
        }
}

size_t hs_hash_map_size(const hs_hash_map *map)
{
        return map->size;
}

bool hs_hash_map_has_key(const hs_hash_map *map, const void *key)
{
        return hs_hash_map_find_bucket(map, key) != NULL;
}

double hs_hash_map_load_factor(const hs_hash_map *map)
{
        return (double) map->size / map->capacity;
}

bool hs_hash_map_is_empty(const hs_hash_map *map)
{
        return map->size == 0;
}

void hs_hash_map_for_each(hs_hash_map *map, hs_iter_func iterator)
{
        for (size_t i = 0; i < map->capacity; ++i) {
                hs_hash_map_bucket *bucket = map->buckets + i;
                if (bucket->has_value)
                        iterator(bucket->key, bucket->value);
        }
}

void hs_hash_map_for_each_const(const hs_hash_map *map,
                                hs_const_iter_func iterator)
{
        for (size_t i = 0; i < map->capacity; ++i) {
                hs_hash_map_bucket *bucket = map->buckets + i;
                if (bucket->has_value)
                        iterator(bucket->key, bucket->value);
        }
}

void hs_hash_map_free(hs_hash_map *map)
{
        if (map->key_remove_notify || map->value_remove_notify) {
                for (size_t i = 0; i < map->capacity; ++i) {
                        hs_hash_map_bucket *bucket = map->buckets + i;
                        if (!bucket->has_value)
                                continue;
                        if (map->key_remove_notify)
                                map->key_remove_notify(bucket->key);
                        if (map->value_remove_notify)
                                map->value_remove_notify(bucket->value);
                }
        }
        free(map->buckets);
        free(map);
}
