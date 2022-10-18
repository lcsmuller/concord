// MIT License
// Copyright (c) 2022 Anotra
// https://github.com/Anotra/anomap

#pragma once

#ifndef ANOMAP_H
#define ANOMAP_H

#include <stddef.h>
#include <stdbool.h>

#define ANOMAP_DECLARE_COMPARE_FUNCTION(function_name, data_type)             \
  static int                                                                  \
  function_name(const void *a, const void *b) {                               \
    if (*(data_type *)a == *(data_type *)b) return 0;                         \
    return *(data_type *)a > *(data_type *)b ? 1 : -1;                        \
  }

enum anomap_operation {
  anomap_insert = 1 << 0,
  anomap_update = 1 << 1,
  anomap_upsert = anomap_insert | anomap_update,
  anomap_delete = 1 << 2,
  anomap_getval = 1 << 3,
};

struct anomap;

struct anomap *anomap_create(size_t key_size, size_t val_size,
                             int (*cmp)(const void *, const void *));
void anomap_destroy(struct anomap *map);

struct anomap_item_changed {
  void *data;
  enum anomap_operation op;
  void *key;
  struct {
    void *prev;
    void *now;
  } val;
};

typedef void anomap_on_item_changed(
  struct anomap *map, struct anomap_item_changed *item_changed);

void anomap_set_on_item_changed(
  struct anomap *map, anomap_on_item_changed *on_changed, void *data);

size_t anomap_length(struct anomap *map);
void anomap_clear(struct anomap *map);

bool anomap_index_of(struct anomap *map, void *key, size_t *index);
bool anomap_at_index(struct anomap *map, size_t index, void *key, void *val);

enum anomap_operation anomap_do(struct anomap *map,
                                enum anomap_operation operation,
                                void *key, void *val);

size_t anomap_copy_range(struct anomap *map,
                         size_t from_index, size_t to_index,
                         void *keys, void *vals);
size_t anomap_delete_range(struct anomap *map,
                           size_t from_index, size_t to_index,
                           void *keys, void *vals);

#endif // !ANOMAP_H
