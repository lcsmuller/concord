// MIT License
// Copyright (c) 2022 Anotra
// https://github.com/Anotra/anomap

#include <stdlib.h>
#include <string.h>

#include "anomap.h"

#define NATIVE_LOCK_NONE     0
#define NATIVE_LOCK_PTHREADS 1
#define NATIVE_LOCK_WINDOWS  2

#if !defined ANOMAP_NATIVE_LOCKS
  #if defined __has_include
    #if __has_include(<pthread.h>)
      #define ANOMAP_NATIVE_LOCKS NATIVE_LOCK_PTHREADS
    #elif __has_include(<synchapi.h>)
      #define ANOMAP_NATIVE_LOCKS NATIVE_LOCK_WINDOWS
    #endif
  #endif
#endif

struct anomap_lock {
  void *(*create)(void);
  void (*destroy)(void *lock);
  struct {
    bool (*attempt)(void *lock);
    void (*acquire)(void *lock);
    void (*release)(void *lock);
  } r, w;
};

static void *
_null_lock_create(void) {
  static int dummy = 0;
  return &dummy;
}

static void
_null_lock_destroy(void *lock) {
  (void)lock;
}

static bool
_null_lock_attempt(void *lock) {
  (void)lock;
  return true;
}

static void
_null_lock_acquire(void *lock) {
  (void)lock;
}

static void
_null_lock_release(void *lock) {
  (void)lock;
}

static const struct anomap_lock null_lock = {
  .create = _null_lock_create,
  .destroy = _null_lock_destroy,
  .r = {
    .attempt = _null_lock_attempt,
    .acquire = _null_lock_acquire,
    .release = _null_lock_release,
  },
  .w = {
    .attempt = _null_lock_attempt,
    .acquire = _null_lock_acquire,
    .release = _null_lock_release,
  }
};

static unsigned anomap_has_locks =
#if !defined ANOMAP_NATIVE_LOCKS || ANOMAP_NATIVE_LOCKS == NATIVE_LOCK_NONE
  0;
#else
  anomap_use_lock;

  #if ANOMAP_NATIVE_LOCKS == NATIVE_LOCK_PTHREADS
    #ifndef _GNU_SOURCE
      #define _GNU_SOURCE
    #endif
    #include <pthread.h>

    static void *
    _pthread_lock_create(void) {
      pthread_rwlock_t *lock = calloc(1, sizeof *lock);
      if (!lock) return NULL;
      pthread_rwlock_init(lock, NULL);
      return lock;
    }

    static void
    _pthread_lock_destroy(void *lock) {
      pthread_rwlock_destroy(lock);
      free(lock);
    }

    static bool
    _pthread_rd_attempt(void *lock) {
      return 0 == pthread_rwlock_tryrdlock(lock);
    }

    static void
    _pthread_rd_acquire(void *lock) {
      pthread_rwlock_rdlock(lock);
    }

    static void
    _pthread_rd_release(void *lock) {
      pthread_rwlock_unlock(lock);
    }

    static bool
    _pthread_wr_attempt(void *lock) {
      return 0 == pthread_rwlock_trywrlock(lock);
    }

    static void
    _pthread_wr_acquire(void *lock) {
      pthread_rwlock_wrlock(lock);
    }

    static void
    _pthread_wr_release(void *lock) {
      pthread_rwlock_unlock(lock);
    }

    static const struct anomap_lock lock_functions = {
      .create = _pthread_lock_create,
      .destroy = _pthread_lock_destroy,
      .r = {
        .attempt = _pthread_rd_attempt,
        .acquire = _pthread_rd_acquire,
        .release = _pthread_rd_release,
      },
      .w = {
        .attempt = _pthread_wr_attempt,
        .acquire = _pthread_wr_acquire,
        .release = _pthread_wr_release,
      },
    };
  #elif ANOMAP_NATIVE_LOCKS == NATIVE_LOCK_WINDOWS
    #include <windows.h>
    #include <synchapi.h>
    static void *
    _srw_lock_create(void) {
      SRWLOCK *lock = calloc(1, sizeof *lock);
      if (!lock) return NULL;
      InitializeSRWLock(lock);
      return lock;
    }

    static void
    _srw_lock_destroy(void *lock) {
      free(lock);
    }

    static bool
    _srw_rd_attempt(void *lock) {
      return TryAcquireSRWLockShared(lock);
    }

    static void
    _srw_rd_acquire(void *lock) {
      AcquireSRWLockShared(lock);
    }

    static void
    _srw_rd_release(void *lock) {
      ReleaseSRWLockShared(lock);
    }

    static bool
    _srw_wr_attempt(void *lock) {
      return TryAcquireSRWLockExclusive(lock);
    }

    static void
    _srw_wr_acquire(void *lock) {
      AcquireSRWLockExclusive(lock);
    }

    static void
    _srw_wr_release(void *lock) {
      ReleaseSRWLockExclusive(lock);
    }

    static const struct anomap_lock lock_functions = {
      .create = _srw_lock_create,
      .destroy = _srw_lock_destroy,
      .r = {
        .attempt = _srw_rd_attempt,
        .acquire = _srw_rd_acquire,
        .release = _srw_rd_release,
      },
      .w = {
        .attempt = _srw_wr_attempt,
        .acquire = _srw_wr_acquire,
        .release = _srw_wr_release,
      },
    };
  #else
    #error native lock not yet implemented
  #endif
#endif

#define LOCK_DO(MODE, FUNC)                        \
  map->lock.functions->MODE.FUNC(map->lock.lock)
#define LOCK_W_ATTEMPT LOCK_DO(w, attempt)
#define LOCK_W_ACQUIRE LOCK_DO(w, acquire)
#define LOCK_W_RELEASE LOCK_DO(w, release)
#define LOCK_R_ATTEMPT LOCK_DO(r, attempt)
#define LOCK_R_ACQUIRE LOCK_DO(r, acquire)
#define LOCK_R_RELEASE LOCK_DO(r, release)

#define ANOMAP_ALLOWED_OPTIONS ( anomap_reverse_order              \
                               | anomap_direct_access              \
                               | anomap_preserve_order             \
                               | anomap_has_locks                  \
                               )

struct anomap {
  int (*cmp)(const void *, const void *);
  anomap_options options;
  bool free_on_cleanup;
  struct {
    void *lock;
    const struct anomap_lock *functions;
  } lock;
  struct {
    anomap_on_item_changed *cb;
    void *data;
  } on_changed;
  struct {
    unsigned *arr;
    size_t len : 32, cap : 32;
    unsigned highest;
  } map;
  struct {
    char *arr;
    size_t cap : 32, size : 32;
  } keys;
  struct {
    char *arr;
    size_t cap : 32, size : 32;
  } vals;
  struct {
    struct {
      unsigned prev, next;
    } *arr;
    size_t cap : 32;
    unsigned tail;
  } order;
};

static void _anomap_foreach(anomap *map, anomap_foreach_cb *cb,
                            void *data, anomap_position start_position);

anomap_options
anomap_supported_options(void) {
  return ANOMAP_ALLOWED_OPTIONS;
}

size_t
anomap_struct_size(void) {
  return sizeof(anomap);
}

bool
anomap_init(anomap *map,
            size_t key_size, size_t val_size,
            int (*cmp)(const void *, const void *),
            anomap_options options)
{
  if (!map || !key_size || !cmp || (options & ~ANOMAP_ALLOWED_OPTIONS))
    return false;
  memset(map, 0, sizeof *map);
  map->free_on_cleanup = false;
  map->options = options;
  map->cmp = cmp;
  map->keys.size = key_size;
  map->vals.size = val_size;
#if !defined ANOMAP_NATIVE_LOCKS || ANOMAP_NATIVE_LOCKS == NATIVE_LOCK_NONE
  map->lock.functions = &null_lock;
#else
  map->lock.functions = options & anomap_use_lock
                      ? &lock_functions
                      : &null_lock;
#endif
  if ((map->lock.lock = map->lock.functions->create()))
    return true;

  memset(map, 0, sizeof *map);
  return false;
}

anomap *
anomap_create(size_t key_size, size_t val_size,
              int (*cmp)(const void *, const void *),
              anomap_options options)
{
  anomap *map = calloc(1, sizeof *map);
  if (!map) return NULL;
  if (anomap_init(map, key_size, val_size, cmp, options))
    return map->free_on_cleanup = true, map;
  return free(map), NULL;
}

void
anomap_destroy(anomap *map) {
  anomap_clear(map);
  free(map->map.arr);
  free(map->keys.arr);
  free(map->vals.arr);
  free(map->order.arr);
  map->lock.functions->destroy(map->lock.lock);
  const bool free_on_cleanup = map->free_on_cleanup;
  memset(map, 0, sizeof *map);
  if (free_on_cleanup)
    free(map);
}

anomap *
anomap_clone(anomap *map, anomap_clone_options options) {
  if (options) return NULL;
  anomap *clone = malloc(sizeof *clone);
  if (!clone) return NULL;
  struct {
    void *ptrs[16];
    size_t len;
  } cleanup = { .len = 0 };

  LOCK_R_ACQUIRE;
  memcpy(clone, map, sizeof *clone);
  clone->free_on_cleanup = true;

  if (!(clone->lock.lock = clone->lock.functions->create()))
    goto lock_create_fail;
  
#define CLONE_ARRAY(ARRAY, SIZE)                      \
  do {                                                \
    if ((clone->ARRAY.arr = malloc(SIZE))) {          \
      memcpy(clone->ARRAY.arr, map->ARRAY.arr, SIZE); \
      cleanup.ptrs[cleanup.len++] = clone->ARRAY.arr; \
    } else goto array_copy_fail;                      \
  } while (0)

  if (clone->map.len) {
    CLONE_ARRAY(map, clone->map.cap * sizeof *clone->map.arr);
    CLONE_ARRAY(keys, clone->keys.cap * clone->keys.size);
    if (clone->vals.size)
      CLONE_ARRAY(vals, clone->vals.cap * clone->vals.size);
    if (clone->options & anomap_preserve_order)
      CLONE_ARRAY(order, clone->order.cap * sizeof *clone->order.arr);
  } else {
    memset(&clone->map, 0, sizeof clone->map);
    memset(&clone->keys, 0, sizeof clone->keys);
    clone->keys.size = map->keys.size;
    memset(&clone->vals, 0, sizeof clone->vals);
    clone->vals.size = map->vals.size;
    memset(&clone->order, 0, sizeof clone->order);
  }

  LOCK_R_RELEASE;
  return clone;

  array_copy_fail:
  while (cleanup.len)
    free(cleanup.ptrs[--cleanup.len]);
  clone->lock.functions->destroy(clone->lock.lock);
  lock_create_fail:
  LOCK_R_RELEASE;
  free(clone);
  return NULL;
}

void
anomap_move(anomap *dest, bool free_on_destroy, anomap *map) {
  LOCK_W_ACQUIRE;
  memcpy(dest, map, sizeof *dest);
  const bool free_map = map->free_on_cleanup;
  dest->free_on_cleanup = free_on_destroy;
  memset(map, 0, sizeof *map);
  if (free_map) free(map);
  map = dest;
  LOCK_W_RELEASE;
}

void
anomap_set_on_item_changed(
  anomap *map, anomap_on_item_changed *on_changed, void *data)
{
  LOCK_W_ACQUIRE;
  map->on_changed.cb = on_changed;
  map->on_changed.data = data;
  LOCK_W_RELEASE;
}

size_t
anomap_length(anomap *map) {
  LOCK_R_ACQUIRE;
  size_t len = map->map.len;
  LOCK_R_RELEASE;
  return len;
}

static inline void
_anomap_on_empty(anomap *map) {
  if (map->map.len) return;
  if (map->options & anomap_preserve_order)
    map->order.tail = map->order.arr[0].next = map->order.arr[0].prev = 0;
  map->map.highest = 0;
}

static void
_anomap_clear_foreach(const void *key, const void *val, void *data) {
  anomap *map = data;
  map->on_changed.cb(
    &(struct anomap_item_changed) {
      .map = map,
      .data = map->on_changed.data,
      .op = anomap_delete,
      .key = (void *)key,
      .val.prev = (void *)val,
    });
}

void
anomap_clear(anomap *map) {
  LOCK_W_ACQUIRE;
  if (map->map.len) {
    if (map->on_changed.cb) {
      if (map->options & anomap_preserve_order)
        _anomap_foreach(map, _anomap_clear_foreach, map, anomap_tail);
      else _anomap_foreach(map, _anomap_clear_foreach, map, anomap_head);
    }
    map->map.len = 0;
    _anomap_on_empty(map);
  }
  LOCK_W_RELEASE;
}

bool
anomap_contains(anomap *map, void *key) {
  size_t position;
  return anomap_index_of(map, key, &position);
}

static bool
anomap_index_of_no_lock(anomap *map, void *key, size_t *position) {
  size_t lo = 0, mid, hi = map->map.len;
  const char *const keys = map->keys.arr;
  const size_t key_size = map->keys.size;
  int result;

  if (0 == map->map.len) goto on_empty;
  result = map->cmp(key, keys + key_size * map->map.arr[map->map.len - 1]);
  if (0 == result) return *position = map->map.len - 1, true;

# define BINARY_SEARCH(cmp_operator)                                          \
  if (result cmp_operator 0)                                                  \
    return *position = map->map.len, false;                                   \
  while (lo < hi) {                                                           \
    mid = lo + (hi - lo) / 2;                                                 \
    result = map->cmp(key, keys + key_size * map->map.arr[mid]);              \
    if (result == 0) return *position = mid, true;                            \
    if (result cmp_operator 0) lo = mid + 1;                                  \
    else hi = mid;                                                            \
  }
  
  if (map->options & anomap_reverse_order) {
    BINARY_SEARCH(<);
  } else {
    BINARY_SEARCH(>);
  }

# undef BINARY_SEARCH

  on_empty:
  return *position = lo, false;
}

bool
anomap_index_of(anomap *map, void *key, size_t *position) {
  LOCK_R_ACQUIRE;
  bool found = anomap_index_of_no_lock(map, key, position);
  LOCK_R_RELEASE;
  return found;
}

bool
anomap_at_index(anomap *map, size_t index, void *key, void *val) {
  LOCK_R_ACQUIRE;
  if (index >= map->map.len) {
    LOCK_R_RELEASE;
    return false;
  }
  const size_t key_size = map->keys.size, val_size = map->vals.size;
  const unsigned pos = map->map.arr[index];
  if (key) memcpy(key, map->keys.arr + key_size * pos, key_size);
  if (val) memcpy(val, map->vals.arr + val_size * pos, val_size);
  LOCK_R_RELEASE;
  return true;
}

const void *
anomap_direct_key_at_index(anomap *map, size_t index) {
  void *result = NULL;
  LOCK_R_ACQUIRE;
  if (!(map->options & anomap_direct_access))
    goto fail;
  if (index >= map->map.len)
    goto fail;

  result = map->keys.arr + map->keys.size * map->map.arr[index];

  fail:
  LOCK_R_RELEASE;
  return result;
}

void *
anomap_direct_val_at_index(anomap *map, size_t index) {
  void *result = NULL;
  LOCK_R_ACQUIRE;
  if (!(map->options & anomap_direct_access))
    goto fail;
  if (index >= map->map.len)
    goto fail;
  if (!map->vals.size)
    goto fail;

  result = map->vals.arr + map->vals.size * map->map.arr[index];

  fail:
  LOCK_R_RELEASE;
  return result;
}

static bool
_anomap_ensure_capacity(anomap *map, size_t capacity) {
  if (capacity > (size_t)1 << 31) return false;
  if (capacity <= map->map.cap) return true;
  size_t cap = map->map.cap ? map->map.cap << 1 : 8;
  while (cap < capacity) if (0 == (cap <<= 1)) return false;

#define RESIZE_ARRAY(ARRAY, ELEMENT_SIZE, CAPACITY, NEW_CAPACITY)         \
  do {                                                                    \
    if (CAPACITY < NEW_CAPACITY) {                                        \
      void *tmp =     /* use calloc initially to be safe */               \
        0 == CAPACITY ? calloc((NEW_CAPACITY), (ELEMENT_SIZE))            \
                      : realloc(ARRAY, (NEW_CAPACITY) *(ELEMENT_SIZE));   \
      if (!tmp) return false;                                             \
      ARRAY = tmp;                                                        \
      CAPACITY = NEW_CAPACITY;                                            \
    }                                                                     \
  } while (0)

  RESIZE_ARRAY(map->map.arr, sizeof *map->map.arr, map->map.cap, cap);

  RESIZE_ARRAY(map->keys.arr, map->keys.size, map->keys.cap, cap);

  if (map->vals.size)
    RESIZE_ARRAY(map->vals.arr, map->vals.size, map->vals.cap, cap);

  if (map->options & anomap_preserve_order)
    RESIZE_ARRAY(map->order.arr, sizeof *map->order.arr, map->order.cap, cap);

  return true;
}

static void
_unlink_element(anomap *map, unsigned pos) {
  if (0 == map->map.len) {
    map->order.tail = map->order.arr[0].next = map->order.arr[0].prev = 0;
  } else {
    const unsigned prev = map->order.arr[pos].prev;
    const unsigned next = map->order.arr[pos].next;
    map->order.arr[next].prev = prev;
    map->order.arr[prev].next = next;
    if (map->order.tail == pos)
      map->order.tail = prev;
  }
}

anomap_operation
anomap_do(anomap *map, anomap_operation operation, void *key, void *val) {
  const bool use_write_lock = operation != anomap_getval;
  if (use_write_lock)
    LOCK_W_ACQUIRE;
  else LOCK_R_ACQUIRE;
  const size_t key_size = map->keys.size, val_size = map->vals.size;
  anomap_operation result = 0;
  size_t mpos = 0;
  if (!anomap_index_of_no_lock(map, key, &mpos)) {
    if (!(operation & anomap_insert))
      goto finish;
    if (!_anomap_ensure_capacity(map, map->map.len + 1))
      goto finish;
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
  
    if (map->options & anomap_preserve_order) {
      unsigned tail = map->order.tail;
      unsigned head = map->order.arr[tail].next;
      map->order.arr[map->order.arr[pos].prev = tail].next = pos;
      map->order.arr[map->order.arr[pos].next = head].prev = pos;
      map->order.tail = pos;
    }

    result |= anomap_insert;
    if (map->on_changed.cb)
      map->on_changed.cb(
        &(struct anomap_item_changed) {
          .map = map,
          .data = map->on_changed.data,
          .op = anomap_insert,
          .key = key,
          .val.now = val,
        });
    goto finish;
  }
  unsigned pos = map->map.arr[mpos];
  if (operation & anomap_update) {
    result |= anomap_update;
    void *const val_to_update = map->vals.arr + val_size * pos;
    if (map->on_changed.cb)
      map->on_changed.cb(
        &(struct anomap_item_changed) {
          .map = map,
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
      for (size_t amount_left = map->vals.size; amount_left;) {
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
      map->on_changed.cb(
        &(struct anomap_item_changed) {
          .map = map,
          .data = map->on_changed.data,
          .op = anomap_delete,
          .key = key,
          .val.prev = deleted_item,
        });
    if (mpos < --map->map.len)
      memmove(map->map.arr + mpos, map->map.arr + mpos + 1,
              sizeof *map->map.arr * (map->map.len - mpos));
    map->map.arr[map->map.len] = pos;
    if (map->options & anomap_preserve_order)
      _unlink_element(map, pos);
    _anomap_on_empty(map);
  }
  finish:
  if (use_write_lock)
    LOCK_W_RELEASE;
  else LOCK_R_RELEASE;
  return result;
}

static size_t
anomap_copy_range_no_lock(anomap *map, 
                          size_t from_index, size_t to_index,
                          void *keys, void *vals)
{
  size_t count = 1 + (from_index < to_index ? to_index - from_index :
                                              from_index - to_index);
  if (from_index >= map->map.len || to_index >= map->map.len) return 0;
  if (keys || vals) {
    const size_t key_size = map->keys.size;
    const size_t val_size = map->vals.size;
    const int next = from_index <= to_index ? 1 : -1;
    for (size_t i = 0;; i++, from_index += next) {
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
anomap_copy_range(anomap *map, size_t from_index, size_t to_index,
                  void *keys, void *vals)
{
  LOCK_R_ACQUIRE;
  size_t count = anomap_copy_range_no_lock(map, from_index, to_index,
                                           keys, vals);
  LOCK_R_RELEASE;
  return count;
}

size_t
anomap_delete_range(anomap *map, size_t from_index, size_t to_index,
                    void *keys, void *vals)
{
  LOCK_W_ACQUIRE;
  size_t count = anomap_copy_range_no_lock(map, from_index, to_index,
                                           keys, vals);
  if (!count) goto finish;
  const int next = from_index <= to_index ? 1 : -1;
  if (map->options & anomap_preserve_order)
    for (size_t i = from_index;; i += next) {
      _unlink_element(map, map->map.arr[i]);
      if (i == to_index) break;
    }
  for (size_t i = from_index;; i += next) {
    if (!map->on_changed.cb) break;
    unsigned pos = map->map.arr[i];
    map->on_changed.cb(
      &(struct anomap_item_changed) {
        .map = map,
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
  _anomap_on_empty(map);
  finish:
  LOCK_W_RELEASE;
  return count;
}

static void
_anomap_foreach(anomap *map, anomap_foreach_cb *cb, void *data,
                anomap_position start_position)
{
  const size_t key_size = map->keys.size, val_size = map->vals.size;
  if (0 == map->map.len)
    return;
  if (map->options & anomap_preserve_order) {
    unsigned tail = map->order.tail;
    unsigned head = map->order.arr[tail].next;
    unsigned start = start_position == anomap_head ? head : tail;
    unsigned pos = start;
    switch ((start_position == anomap_tail ? 1 : 0)) {
      while (pos != tail) {
        pos = map->order.arr[pos].next;
        case 0:
        cb(map->keys.arr + key_size * pos,
           map->vals.arr + val_size * pos,
           data);
      } break;
      while (pos != head) {
        pos = map->order.arr[pos].prev;
        case 1:
        cb(map->keys.arr + key_size * pos,
           map->vals.arr + val_size * pos,
           data);
      } break;
    }
  } else {
    if (start_position == anomap_head) {
      for (size_t i=0; i<map->map.len; i++)
        cb(map->keys.arr + key_size * map->map.arr[i],
           map->vals.arr + val_size * map->map.arr[i],
           data);
    } else {
      for (size_t i = map->map.len; i--;)
        cb(map->keys.arr + key_size * map->map.arr[i],
           map->vals.arr + val_size * map->map.arr[i],
           data);
    }
  } 
}

void
anomap_foreach(anomap *map, anomap_foreach_cb *cb, void *data) {
  LOCK_R_ACQUIRE;
  _anomap_foreach(map, cb, data, anomap_head);
  LOCK_R_RELEASE;
}

void
anomap_foreach_reverse(anomap *map, anomap_foreach_cb *cb, void *data) {
  LOCK_R_ACQUIRE;
  _anomap_foreach(map, cb, data, anomap_tail);
  LOCK_R_RELEASE;
}


bool
anomap_advance(anomap *map, size_t *index, anomap_position *position) {
  unsigned pos;
  LOCK_R_ACQUIRE;
  if (0 == map->map.len)
    goto return_false;
  if (map->options & anomap_preserve_order) {
    switch (*position) {
      case anomap_head:
        pos = map->order.arr[map->order.tail].next;
        anomap_index_of(map, map->keys.arr + map->keys.size * pos, index);
        *position = anomap_next;
        goto return_true;
      case anomap_tail:
        pos = map->order.tail;
        anomap_index_of(map, map->keys.arr + map->keys.size * pos, index);
        *position = anomap_prev;
        goto return_true;
      case anomap_next:
        pos = map->map.arr[*index];
        if (pos == map->order.tail)
          goto return_false;
        pos = map->order.arr[pos].next;
        anomap_index_of(map, map->keys.arr + map->keys.size * pos, index);
        goto return_true;
      case anomap_prev:
        pos = map->map.arr[*index];
        if (pos == map->order.arr[map->order.tail].next)
          goto return_false;
        pos = map->order.arr[pos].prev;
        anomap_index_of(map, map->keys.arr + map->keys.size * pos, index);
        goto return_true;
    }
  } else {
    switch (*position) {
      case anomap_head:
        *index = 0;
        *position = anomap_next;
        goto return_true;
      case anomap_tail:
        *index = map->map.len - 1;
        *position = anomap_prev;
        goto return_true;
      case anomap_next:
        if (*index >= map->map.len - 1)
          goto return_false;
        (*index)++;
        goto return_true;
      case anomap_prev:
        if (0 == *index)
          goto return_false;
        (*index)--;
        goto return_true;
    }
  }
  // false should be first as it's the default return value
  return_false:
  LOCK_R_RELEASE;
  return false;

  return_true:
  LOCK_R_RELEASE;
  return true;
}

int
anomap_cmp_str(const void *a, const void *b) {
  return strcmp(*(char **)a, *(char **)b);
}
