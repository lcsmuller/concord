// MIT License
// Copyright (c) 2022 Anotra
// https://github.com/Anotra/anomap

#include <stdlib.h>
#include <string.h>

#include "anomap.h"

struct anomap {
  int (*cmp)(const void *, const void *);
  struct {
    anomap_on_item_changed *cb;
    void *data;
  } on_changed;
  struct {
    unsigned *arr;
    size_t len, cap;
    size_t highest;
  } map;
  struct {
    char *arr;
    size_t cap, size;
  } keys;
  struct {
    char *arr;
    size_t cap, size;
  } vals;
};

struct anomap *
anomap_create(size_t key_size, size_t val_size,
              int (*cmp)(const void *, const void *)) {
  struct anomap *map = calloc(1, sizeof *map);
  if (map) {
    map->cmp = cmp;
    map->keys.size = key_size;
    map->vals.size = val_size;
    return map;
  }
  return NULL;
}

void
anomap_destroy(struct anomap *map) {
  anomap_clear(map);
  free(map->keys.arr);
  free(map->vals.arr);
  free(map->map.arr);
  memset(map, 0, sizeof *map);
  free(map);
}

void
anomap_set_on_item_changed(
  struct anomap *map, anomap_on_item_changed *on_changed, void *data)
{
  map->on_changed.cb = on_changed;
  map->on_changed.data = data;
}

size_t
anomap_length(struct anomap *map) {
  return map->map.len;
}

void anomap_clear(struct anomap *map) {
  for (size_t i = 0; i < map->map.len; i++) {
    if (!map->on_changed.cb) break;
    unsigned pos = map->map.arr[i];
    map->on_changed.cb(map,
      &(struct anomap_item_changed) {
        .data = map->on_changed.data,
        .op = anomap_delete,
        .key = map->keys.arr + map->keys.size * pos,
        .val.prev = map->vals.arr + map->vals.size * pos,
      });
  }
  map->map.len = 0;
  map->map.highest = 0;
}

bool
anomap_index_of(struct anomap *map, void *key, size_t *position) {
  size_t lo = 0, mid, hi = map->map.len;
  const char *const keys = map->keys.arr;
  const size_t key_size = map->keys.size;

  if (map->map.len >= 0x10 // support fast appending
   && map->cmp(key, keys + key_size * map->map.arr[map->map.len - 1]) > 0)
    return *position = map->map.len, false;

  while (lo < hi) {
    mid = lo + (hi - lo) / 2;
    int r = map->cmp(key, keys + key_size * map->map.arr[mid]);
    if (r == 0) return *position = mid, true;
    if (r > 0) lo = mid + 1;
    else hi = mid;
  }
  return *position = lo, false;
}

bool
anomap_at_index(struct anomap *map, size_t index, void *key, void *val) {
  if (index >= map->map.len) return false;
  const size_t key_size = map->keys.size, val_size = map->vals.size;
  const unsigned pos = map->map.arr[index];
  if (key) memcpy(key, map->keys.arr + key_size * pos, key_size);
  if (val) memcpy(val, map->vals.arr + val_size * pos, val_size);
  return true;
}

static bool
_anomap_ensure_capacity(struct anomap *map, size_t capacity) {
  if (capacity > ~(unsigned)0) return false;
  if (capacity <= map->map.cap) return true;
  size_t cap = map->map.cap ? map->map.cap << 1 : 8;
  while (cap < capacity) cap <<= 1;
  if (map->keys.cap < cap) {
    void *tmp = realloc(map->keys.arr, map->keys.size * cap);
    if (!tmp) return false;
    map->keys.arr = tmp;
    map->keys.cap = cap;
  }
  if (map->vals.size && map->vals.cap < cap) {
    void *tmp = realloc(map->vals.arr, map->vals.size * cap);
    if (!tmp) return false;
    map->vals.arr = tmp;
    map->vals.cap = cap;
  }
  if (map->map.cap < cap) {
    unsigned *tmp = realloc(map->map.arr, sizeof *map->map.arr * cap);
    if (!tmp) return false;
    map->map.arr = tmp;
    map->map.cap = cap;
  }
  return true;
}

enum anomap_operation
anomap_do(struct anomap *map, enum anomap_operation operation,
          void *key, void *val)
{
  const size_t key_size = map->keys.size, val_size = map->vals.size;
  enum anomap_operation result = 0;
  size_t mpos = 0;
  if (!anomap_index_of(map, key, &mpos)) {
    if (~operation & anomap_insert)
      return 0;
    if (!_anomap_ensure_capacity(map, map->map.len + 1))
      return 0;
    unsigned pos = map->map.highest == map->map.len
                 ? map->map.highest++
                 : map->map.arr[map->map.len];
    memcpy(map->keys.arr + key_size * pos, key, key_size);
    if (map->vals.size) {
      if (val) memcpy(map->vals.arr + val_size * pos, val, val_size);
      else memset(map->vals.arr + val_size * pos, 0, val_size);
    }
    if (mpos < map->map.len)
      memmove(map->map.arr + mpos + 1, map->map.arr + mpos,
              sizeof *map->map.arr * (map->map.len - mpos));
    map->map.arr[mpos] = pos;
    map->map.len++;
    result |= anomap_insert;
    if (map->on_changed.cb)
      map->on_changed.cb(map,
        &(struct anomap_item_changed) {
          .data = map->on_changed.data,
          .op = anomap_insert,
          .key = key,
          .val.now = val,
        });
    return result;
  }
  unsigned pos = map->map.arr[mpos];
  if (operation & anomap_update) {
    result |= anomap_update;
    void *const val_to_update = map->vals.arr + val_size * pos;
    if (map->on_changed.cb)
      map->on_changed.cb(map,
        &(struct anomap_item_changed) {
          .data = map->on_changed.data,
          .op = anomap_update,
          .key = key,
          .val.prev = val_to_update,
          .val.now = val,
        });
    if (operation & anomap_getval) {
      result |= anomap_getval;
      char tmp[0x1000];
      char *a = val_to_update;
      char *b = val;
      size_t amount_left = map->vals.size;
      for (size_t i = 0; amount_left; i += sizeof tmp) {
        size_t current_block = amount_left;
        if (current_block > sizeof tmp) current_block = sizeof tmp;
        memcpy(tmp, a, current_block);
        memcpy(a, b, current_block);
        memcpy(b, tmp, current_block);
        amount_left -= current_block;
        a += current_block;
        b += current_block;
      }
    } else {
      if (val_size) {
        if (val) memcpy(val_to_update, val, val_size);
        else memset(val_to_update, 0, val_size);
      }
    }
  } else if (operation & anomap_getval) {
    result |= anomap_getval;
    memcpy(val, map->vals.arr + val_size * pos, val_size);
  }

  if (operation & anomap_delete) {
    result |= anomap_delete;
    void *const deleted_item = map->vals.arr + val_size * pos;
    if (map->on_changed.cb)
      map->on_changed.cb(map,
        &(struct anomap_item_changed) {
          .data = map->on_changed.data,
          .op = anomap_delete,
          .key = key,
          .val.prev = deleted_item,
        });
    if (mpos < --map->map.len)
      memmove(map->map.arr + mpos, map->map.arr + mpos + 1,
              sizeof *map->map.arr * (map->map.len - mpos));
    map->map.arr[map->map.len] = pos;
  }
  return result;
}

size_t
anomap_copy_range(struct anomap *map, size_t from_index, size_t to_index,
                  void *keys, void *vals)
{
  size_t count = 1 + (from_index < to_index ? to_index - from_index :
                                              from_index - to_index);
  if (from_index >= map->map.len || to_index >= map->map.len) return 0;
  if (keys || vals) {
    const size_t key_size = map->keys.size;
    const size_t val_size = map->vals.size;
    bool going_up = from_index <= to_index;
    for (size_t i = 0;; i++, going_up ? from_index++ : from_index--) {
      unsigned pos = map->map.arr[from_index];
      if (keys) memcpy(((char *)keys) + key_size * i,
                        map->keys.arr + key_size * pos,
                        key_size);
      if (vals) memcpy(((char *)vals) + val_size * i,
                        map->vals.arr + val_size * pos,
                        val_size);
      if (from_index == to_index) break;
    }
  }
  return count;
}

size_t
anomap_delete_range(struct anomap *map, size_t from_index, size_t to_index,
                    void *keys, void *vals)
{
  size_t count = anomap_copy_range(map, from_index, to_index, keys, vals);
  if (!count) return 0;
  bool going_up = from_index <= to_index;
  for (size_t i = from_index;; going_up ? i++ : i--) {
    if (!map->on_changed.cb) break;
    unsigned pos = map->map.arr[i];
    map->on_changed.cb(map,
      &(struct anomap_item_changed) {
        .data = map->on_changed.data,
        .op = anomap_delete,
        .key = map->keys.arr + map->keys.size * pos,
        .val.prev = map->vals.arr + map->vals.size * pos,
      });
    if (i == to_index) break;
  }
  
  size_t index = to_index < from_index ? to_index : from_index;
  size_t remaining = count;
  while (remaining) {
    unsigned tmp[4096];
    size_t block = remaining > 4096 ? 4096 : remaining;
    size_t copy_size = block * sizeof *map->map.arr;
    memcpy(tmp, map->map.arr + index, copy_size);
    memmove(map->map.arr + index, map->map.arr + index + block,
            (map->map.len - index - block) * sizeof *map->map.arr);
    map->map.len -= block;
    memcpy(map->map.arr + map->map.len, tmp, copy_size);
    remaining -= block;
  }
  return count;
}
