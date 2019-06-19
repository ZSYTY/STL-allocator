#ifndef ALLOCATOR_HPP
#define ALLOCATOR_HPP

#include <cstdlib>
#include <new>

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
    
    void deallocate(T* p, std::size_t) noexcept 
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
