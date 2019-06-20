#ifndef ALLOCATOR_HPP
#define ALLOCATOR_HPP

#include <cstdlib>
#include <new>
#include <cstddef>
#include <climits>
#include <iostream>
#define MAX_BYTES 128
#define BOUND 8


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
}
/*
template <bool threads, int inst>
class __sec_alloc
{
	;
}
*/

template <class T>
struct Mallocator
{
   	typedef T value_type;
    
    Mallocator() = default;
    
    template <class U>
    Mallocator (const Mallocator<U>& other) noexcept;
    
    T *allocate(std::size_t n) 
	{
        if (n > std::size_t(-1) / sizeof(T))
            throw std::bad_alloc();
        if (auto p = static_cast<T*>(std::malloc(n * sizeof(T))))
            return p;
        throw std::bad_alloc();
    }
    
    void deallocate(T* p, std::size_t)
	{
        std::free(p);
    }
};

template <class T1, class T2>
bool operator == (const Mallocator<T1>& lhs, const Mallocator<T2>& rhs) 
{
	return true;
}

template <class T1, class T2>
bool operator != (const Mallocator<T1>& lhs, const Mallocator<T2> &rhs) {
	return false;
}

#endif
