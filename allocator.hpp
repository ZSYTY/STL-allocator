#ifndef ALLOCATOR_HPP
#define ALLOCATOR_HPP

#include <cstdlib>
#include <new>
#include <cstddef>
#include <climits>
#include <iostream>
#define MAX_BYTES 128
#define BOUND 8
#define COUNT_FREE_LISTS 16


union obj
{
	union obj *nxt;
	char client_data[1];
};

template <int inst>
class __fir_alloc
{
/*	private:
		static void *oom_malloc(std::size_t);
		static void *oom_realloc(void *, std::size_t);
		static void (* __fir_alloc_oom_handler) ();
*/	
	public:
		static void *allocate(std::size_t n)
		{
			void *res = std::malloc(n);
			if (res == 0)
				throw std::bad_alloc();

//				res = oom_malloc(n);
			return res;
		}

		static void deallocate(void *p, std::size_t)
		{
			std::free(p);
		}
};


template <bool threads, int inst>
class __sec_alloc
{
/*	private:
		union obj
		{
			union obj *nxt;
			char client_data[1];
		};
*/
	private:
		static obj *volatile free_list[COUNT_FREE_LISTS];
		static std::size_t free_list_idx(std::size_t bytes)
		{
			return ((bytes) + BOUND - 1) / BOUND - 1;
		}

		static std::size_t ceil_up(std::size_t bytes)
		{
			return (((bytes) + BOUND - 1) & ~(BOUND - 1));
		}

		static void *refill(std::size_t n);

		static char *chunk_alloc(std::size_t size, int &cnt_obj);

		static char *start_mem_pool;
		static char *end_mem_pool;
		static std::size_t heap_size;

	public:
		static void *allocate(std::size_t n)
		{
			obj *volatile *cur_free_list;
			obj *res;
			if (n > (std::size_t) MAX_BYTES)
				return __fir_alloc<0>::allocate(n);

			cur_free_list = free_list[free_list_idx(n)];
			res = *cur_free_list;
			if (res == 0)
			{
				void *r = refill(ceil_up(n));
				return r;
			}
			*cur_free_list = res->nxt;
			return res;
		}
		
		static void deallocate(void *p, std::size_t n)
		{
			obj *q = (obj *)p;
			obj *volatile *cur_free_list;
			if (n > (std::size_t) MAX_BYTES)
			{
				__fir_alloc<0>::deallocate(p, n);
				return ;
			}

			cur_free_list = free_list[free_list_idx(n)];
			q->nxt = *cur_free_list;
			*cur_free_list = q;
		}

		static void *reallocate(void *p, std::size_t old_size, std::size_t new_size);

};

template <bool threads, int inst>
char *__sec_alloc <threads, inst> :: start_mem_pool = 0;

template <bool threads, int inst>
char *__sec_alloc <threads, inst> :: end_mem_pool = 0;

template <bool threads, int inst>
std::size_t __sec_alloc <threads, inst> :: heap_size = 0;

template <bool threads, int inst>
obj *volatile __sec_alloc <threads, inst> :: free_list[COUNT_FREE_LISTS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};


template <class T>
struct Mallocator
{
   	typedef T value_type;
    
    Mallocator() = default;
    
    template <class U>
    Mallocator (const Mallocator <U>& other) noexcept;
    
    T *allocate(std::size_t n) 
	{
        if (n > std::size_t(-1) / sizeof(T))
            throw std::bad_alloc();
        if (auto p = static_cast <T*> (std::malloc(n * sizeof(T))))
            return p;
        throw std::bad_alloc();
    }
    
    void deallocate(T* p, std::size_t)
	{
        std::free(p);
    }
};

template <class T1, class T2>
bool operator == (const Mallocator <T1> & lhs, const Mallocator <T2>& rhs) 
{
	return true;
}

template <class T1, class T2>
bool operator != (const Mallocator <T1> & lhs, const Mallocator <T2>& rhs) {
	return false;
}

#endif
