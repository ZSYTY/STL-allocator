#ifndef ALLOCATOR_HPP
#define ALLOCATOR_HPP

#include <climits>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <new>
enum { MAX_BYTES = 128 };
enum { BOUND = 8 };
enum { COUNT_FREE_LISTS = 16 };

union obj {
    union obj *free_list_link;
    char client_data[1];
};

class __fir_alloc {
   public:
    static void *allocate(std::size_t n) {
        void *res = std::malloc(n);
        if (res == NULL) throw std::bad_alloc();
        return res;
    }

    static void deallocate(void *p, std::size_t) { std::free(p); }
};

class __sec_alloc {
   private:
    static obj *volatile free_list[COUNT_FREE_LISTS];
    static char *start_mem_pool;
    static char *end_mem_pool;
    static std::size_t heap_size;

    static std::size_t free_list_idx(std::size_t bytes) {
        return ((bytes) + BOUND - 1) / BOUND - 1;
    }

    static std::size_t ceil_up(std::size_t bytes) {
        return (((bytes) + BOUND - 1) & ~(BOUND - 1));
    }

    static char *chunk_alloc(std::size_t size, int &cnt_obj) {
        char *res;
        std::size_t total_bytes = size * cnt_obj;
        std::size_t bytes_left = end_mem_pool - start_mem_pool;
        if (bytes_left >= total_bytes) {
            res = start_mem_pool;
            start_mem_pool += total_bytes;
            return res;
        } else if (bytes_left >= size) {
            cnt_obj = bytes_left / size;
            total_bytes = size * cnt_obj;
            res = start_mem_pool;
            start_mem_pool += total_bytes;
            return res;
        } else {
            std::size_t bytes_to_get =
                2 * total_bytes + ceil_up(heap_size >> 4);
            if (bytes_to_get > 0 and start_mem_pool != NULL) {
                obj *volatile *my_free_list =
                    free_list + free_list_idx(bytes_left);
                ((obj *)start_mem_pool)->free_list_link = *my_free_list;
                *my_free_list = (obj *)start_mem_pool;
            }
            start_mem_pool = (char *)malloc(bytes_to_get);
            if (start_mem_pool == NULL) {
                obj *volatile *my_free_list, *p;
                for (int i = size; i <= MAX_BYTES; i += BOUND) {
                    my_free_list = free_list + free_list_idx(i);
                    p = *my_free_list;
                    if (p != NULL) {
                        *my_free_list = p->free_list_link;
                        start_mem_pool = (char *)p;
                        end_mem_pool = start_mem_pool + i;
                        return chunk_alloc(size, cnt_obj);
                    }
                }
                end_mem_pool = 0;
                start_mem_pool = (char *)__fir_alloc::allocate(bytes_to_get);
            }
            heap_size += bytes_to_get;
            end_mem_pool = start_mem_pool + bytes_to_get;
            return chunk_alloc(size, cnt_obj);
        }
    }

    static void *refill(std::size_t n) {
        int cnt_obj = 20;
        char *chunk =
            chunk_alloc(n, cnt_obj);  // cnt_obj is passed by reference
        obj *volatile *my_free_list;
        obj *res;
        obj *current_obj, *next_obj;
        if (cnt_obj == 1) return chunk;
        my_free_list = free_list + free_list_idx(n);
        res = (obj *)chunk;
        *my_free_list = next_obj = (obj *)(chunk + n);

        for (int i = 1;; i++) {
            current_obj = next_obj;
            next_obj = (obj *)((char *)next_obj + n);
            if (cnt_obj - 1 == i) {
                current_obj->free_list_link = NULL;
                break;
            } else {
                current_obj->free_list_link = next_obj;
            }
        }
        return res;
    }

   public:
    static void *allocate(std::size_t n) {
        obj *volatile *cur_free_list;
        obj *res;
        if (n > (std::size_t)MAX_BYTES) return __fir_alloc::allocate(n);

        cur_free_list = free_list + free_list_idx(n);
        res = *cur_free_list;
        if (res == NULL) {
            void *r = refill(ceil_up(n));
            return r;
        }
        *cur_free_list = res->free_list_link;
        return res;
    }

    static void deallocate(void *p, std::size_t n) {
        obj *q = (obj *)p;
        obj *volatile *cur_free_list;
        if (n > (std::size_t)MAX_BYTES) {
            __fir_alloc::deallocate(p, n);
            return;
        }

        cur_free_list = free_list + free_list_idx(n);
        q->free_list_link = *cur_free_list;
        *cur_free_list = q;
    }

    // static void *reallocate(void *p, std::size_t old_size,
    //                         std::size_t new_size);
};

char *__sec_alloc ::start_mem_pool = NULL;

char *__sec_alloc ::end_mem_pool = NULL;

std::size_t __sec_alloc::heap_size = 0;

obj *volatile __sec_alloc::free_list[COUNT_FREE_LISTS] = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

template <class T>
struct Mallocator {
    typedef T value_type;

    Mallocator() = default;

    template <class U>
    Mallocator(const Mallocator<U> &other) noexcept;

    T *allocate(std::size_t n) {
        if (n > std::size_t(-1) / sizeof(T)) throw std::bad_alloc();
        if (auto p = static_cast<T *>(__sec_alloc::allocate(n * sizeof(T))))
            return p;
        throw std::bad_alloc();
    }

    void deallocate(T *p, std::size_t n) {
        __sec_alloc::deallocate(p, n);
    }
};

template <class T1, class T2>
bool operator==(const Mallocator<T1> &lhs, const Mallocator<T2> &rhs) {
    return true;
}

template <class T1, class T2>
bool operator!=(const Mallocator<T1> &lhs, const Mallocator<T2> &rhs) {
    return false;
}

#endif
