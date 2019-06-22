#ifndef ALLOCATOR_HPP
#define ALLOCATOR_HPP

#include <climits>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <new>

// malloc-based allocator
class __fir_alloc {
   public:
    static void *allocate(std::size_t n) {
        void *res = std::malloc(n);
        if (res == nullptr) throw std::bad_alloc();
        return res;
    }

    static void deallocate(void *p, std::size_t) { std::free(p); }
};

// the max size of small block in memory pool
enum { MAX_BYTES = 65536 };

// the align size of each block, i.e. the size could be divided by 64
enum { BOUND = 64 };

// the number of free lists
enum { COUNT_FREE_LISTS = MAX_BYTES / BOUND };

// the union object of free list
union obj {
    union obj *free_list_link;
    char client_data[1];
};

// the second allocator using memory pool
class __sec_alloc {
   private:
    // free_lists
    static obj *volatile free_list[COUNT_FREE_LISTS];

    // chunk allocation state
    static char *start_mem_pool;
    static char *end_mem_pool;
    static std::size_t heap_size;

    // determine the index of free list
    static std::size_t free_list_idx(std::size_t bytes) {
        return ((bytes) + BOUND - 1) / BOUND - 1;
    }

    // align the block
    static std::size_t ceil_up(std::size_t bytes) {
        return (((bytes) + BOUND - 1) & ~(BOUND - 1));
    }

    // allocate using the memory pool
    static char *chunk_alloc(std::size_t size, int &cnt_obj) {
        char *res;
        std::size_t total_bytes = size * cnt_obj;
        // the space left in the memory pool
        std::size_t bytes_left = end_mem_pool - start_mem_pool;

        if (bytes_left >= total_bytes) {
            // if the space left is enough
            res = start_mem_pool;
            start_mem_pool += total_bytes;
            return res;
        } else if (bytes_left >= size) {
            // the space is not enough for requirement
            // but enough for at least one block
            cnt_obj = bytes_left / size;
            total_bytes = size * cnt_obj;
            res = start_mem_pool;
            start_mem_pool += total_bytes;
            return res;
        } else {
            // the space is even not enough for one block
            std::size_t bytes_to_get =
                2 * total_bytes + ceil_up(heap_size >> 4);
            // try to make use of the scattered space
            if (bytes_to_get > 0 and start_mem_pool != nullptr) {
                // if there is still some space
                obj *volatile *my_free_list =
                    free_list + free_list_idx(bytes_left);
                // adjust the free list
                ((obj *)start_mem_pool)->free_list_link = *my_free_list;
                *my_free_list = (obj *)start_mem_pool;
            }

            // supply the memory pool
            start_mem_pool = (char *)malloc(bytes_to_get);
            heap_size += bytes_to_get;
            end_mem_pool = start_mem_pool + bytes_to_get;

            // adjust the cnt_obj
            return chunk_alloc(size, cnt_obj);
        }
    }

    // return an object consuming the space of n
    static void *refill(std::size_t n) {
        int cnt_obj = 20;
        // cnt_obj is passed by reference
        char *chunk = chunk_alloc(n, cnt_obj);
        obj *volatile *my_free_list;
        obj *res;
        obj *current_obj, *next_obj;

        // if get only one block, return it
        if (cnt_obj == 1) return chunk;
        // prepare to adjust free list
        my_free_list = free_list + free_list_idx(n);
        // the block to be returned
        res = (obj *)chunk;
        *my_free_list = next_obj = (obj *)(chunk + n);

        // link the remaining block
        for (int i = 1;; i++) {
            current_obj = next_obj;
            next_obj = (obj *)((char *)next_obj + n);
            if (cnt_obj - 1 == i) {
                current_obj->free_list_link = nullptr;
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

        // if n is large enough call the first allocator
        if (n > (std::size_t)MAX_BYTES) return __fir_alloc::allocate(n);

        // find the suitable free list
        cur_free_list = free_list + free_list_idx(n);
        res = *cur_free_list;

        // refill the free list if there is no free list available
        if (res == nullptr) {
            void *r = refill(ceil_up(n));
            return r;
        }

        // adjust the free list
        *cur_free_list = res->free_list_link;
        return res;
    }

    static void deallocate(void *p, std::size_t n) {
        obj *q = (obj *)p;
        obj *volatile *cur_free_list;

        // if n is large enough, call the first deallocator
        if (n > (std::size_t)MAX_BYTES) {
            __fir_alloc::deallocate(p, n);
            return;
        }

        // find the suitable free list
        cur_free_list = free_list + free_list_idx(n);

        // adjust the free list
        q->free_list_link = *cur_free_list;
        *cur_free_list = q;
    }
};

// initialize the second allocator
char *__sec_alloc ::start_mem_pool = nullptr;

char *__sec_alloc ::end_mem_pool = nullptr;

std::size_t __sec_alloc::heap_size = 0;

obj *volatile __sec_alloc::free_list[COUNT_FREE_LISTS] = {nullptr};

// the interface of allocator
template <class T>
class Mallocator {
   public:
    typedef void _Not_user_specialized;
    typedef T value_type;
    typedef value_type *pointer;
    typedef const value_type *const_pointer;
    typedef value_type &reference;
    typedef const value_type &const_reference;
    typedef std::size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef std::true_type propagate_on_container_move_assignment;
    typedef std::true_type is_always_equal;

    Mallocator() = default;

    template <class U>
    Mallocator(const Mallocator<U> &other) noexcept;

    pointer address(reference _Val) const noexcept { return &_Val; }

    const_pointer address(const_reference _Val) const noexcept { return &_Val; }

    pointer allocate(size_type _Count) {
        if (_Count > size_type(-1) / sizeof(value_type)) throw std::bad_alloc();
        if (auto p = static_cast<pointer>(
                __sec_alloc::allocate(_Count * sizeof(value_type))))
            return p;
        throw std::bad_alloc();
    }

    void deallocate(pointer _Ptr, size_type _Count) {
        __sec_alloc::deallocate(_Ptr, _Count);
    }

    template <class _Uty>
    void destroy(_Uty *_Ptr) {
        _Ptr->~_Uty();
    }

    template <class _Objty, class... _Types>
    void construct(_Objty *_Ptr, _Types &&... _Args) {
        ::new (const_cast<void *>(static_cast<const volatile void *>(_Ptr)))
            _Objty(std::forward<_Types>(_Args)...);
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
