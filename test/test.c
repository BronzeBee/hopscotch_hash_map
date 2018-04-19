#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <hs_hash_map/hs_hash_map.h>
#include <stdlib.h>

char string_keys[4096 * 32];
char string_values[4096 * 32];

size_t djb_hash(const void *data)
{
        const char *string = (const char *) data;
        size_t hash = 5381;
        while (*string)
                hash = 33 * hash ^ (unsigned char) *string++;
        return hash;
}

bool string_equal_func(const void *first, const void *second)
{
        return strcmp((const char *) first, (const char *) second) == 0;
}

void bool_free_func(void *data)
{
        *((bool *) data) = true;
}

void read_lines(const char *file_name, int max_length, char *out)
{
        FILE *file = fopen(file_name, "r");
        assert((file != NULL, "Unable to open input file"));
        size_t i = 0;
        while (fgets(out + (i * max_length), max_length, file))
                i++;
}

int main()
{
        read_lines("string_keys.txt", 32, string_keys);
        read_lines("string_values.txt", 32, string_values);

        /*
         * Single element insertion
         */
        {
                char *key = "This is a test key";
                char *value = "This is a test value";
                hs_hash_map *map = hs_hash_map_new(djb_hash, string_equal_func);
                assert(hs_hash_map_size(map) == 0);
                hs_hash_map_put(map, key, value);
                assert(hs_hash_map_size(map) == 1);
                assert(hs_hash_map_get(map, key) == value);
                hs_hash_map_free(map);
        }

        /*
         * Multiple element insertion with rehash
         */
        {
                hs_hash_map *map = hs_hash_map_new(djb_hash, string_equal_func);
                size_t n = 4096;
                for (size_t i = 0; i < n; ++i) {
                        hs_hash_map_put(map, string_keys + i * 32,
                                        string_values + i * 32);
                        assert(hs_hash_map_size(map) == i + 1);
                }
                for (size_t i = 0; i < n; ++i) {
                        assert(hs_hash_map_get(map, string_keys + i * 32) ==
                               string_values + i * 32);
                }
                hs_hash_map_free(map);
        }

        /*
         * Element removal
         */
        {
                hs_hash_map *map = hs_hash_map_new(djb_hash, string_equal_func);
                size_t n = 2048;
                for (size_t i = 0; i < n; ++i)
                        hs_hash_map_put(map, string_keys + i * 32,
                                        string_values + i * 32);
                assert(hs_hash_map_size(map) == n);
                hs_hash_map_remove(map, string_keys + 4 * 32);
                assert(hs_hash_map_size(map) == n - 1);
                assert(!hs_hash_map_has_key(map, string_keys + 4 * 32));
        }

        /*
         * Value removal handlers
         */
        {
                size_t n = 1024;
                bool values[1024] = {false};
                bool test = false;
                hs_hash_map *map = hs_hash_map_new_extended(djb_hash,
                                                            string_equal_func,
                                                            NULL,
                                                            bool_free_func);
                hs_hash_map_put(map, string_keys + 4, &test);
                for (size_t i = 0; i < n; ++i)
                        hs_hash_map_put(map, string_keys + i, values + i);
                // Was remove handler called for overwritten value?
                assert(test == true);
                hs_hash_map_free(map);
                for (size_t i = 0; i < n; ++i)
                        assert(values[i] == true);
        }

        /**
         * Keys access
         */
        {
                hs_hash_map *map = hs_hash_map_new(djb_hash, string_equal_func);
                size_t n = 1024;
                for (size_t i = 0; i < n; ++i)
                        hs_hash_map_put(map, string_keys + i * 32,
                                        string_values + i * 32);
                void **keys = malloc(sizeof(void *) * n);
                hs_hash_map_get_keys(map, keys);
                size_t matched_keys = 0;
                for (size_t i = 0; i < n; ++i) {
                        for (size_t j = 0; j < n; ++j)
                                if (keys[i] == string_keys + j * 32) {
                                        ++matched_keys;
                                        break;
                                }
                }
                assert(matched_keys == n);
                free(keys);
                hs_hash_map_free(map);
        }

        /**
         * Values access
         */
        {
                hs_hash_map *map = hs_hash_map_new(djb_hash, string_equal_func);
                size_t n = 1024;
                for (size_t i = 0; i < n; ++i)
                        hs_hash_map_put(map, string_keys + i * 32,
                                        string_values + i * 32);
                void **values = malloc(sizeof(void *) * n);
                hs_hash_map_get_values(map, values);
                size_t matched_values = 0;
                for (size_t i = 0; i < n; ++i) {
                        for (size_t j = 0; j < n; ++j)
                                if (values[i] == string_values + j * 32) {
                                        ++matched_values;
                                        break;
                                }
                }
                assert(matched_values == n);
                free(values);
                hs_hash_map_free(map);
        }

        /**
         * Entries access
         */
        {
                hs_hash_map *map = hs_hash_map_new(djb_hash, string_equal_func);
                size_t n = 1024;
                for (size_t i = 0; i < n; ++i)
                        hs_hash_map_put(map, string_keys + i * 32,
                                        string_values + i * 32);
                void **entries = malloc(sizeof(void *) * n * 2);
                hs_hash_map_get_entries(map, entries);
                size_t matched_entries = 0;
                for (size_t i = 0; i < n * 2; i += 2) {
                        for (size_t j = 0; j < n; ++j)
                                if (entries[i] == string_keys + j * 32 &&
                                    entries[i + 1] == string_values + j * 32) {
                                        ++matched_entries;
                                        break;
                                }
                }
                assert(matched_entries == n);
                free(entries);
                hs_hash_map_free(map);
        }

}
