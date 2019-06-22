#ifndef NAIVEALLOCATOR_HPP
#define NAIVEALLOCATOR_HPP

#include <climits>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <new>

template <class T>
class Mallocator {
   public:
    typedef T value_type;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;

    Mallocator() = default;

    template <class U>
    Mallocator(const Mallocator<U>& other) noexcept;

    T* allocate(std::size_t n) {
        auto buf = (T*)(::operator new((std::size_t)(n * sizeof(T))));
        if (buf == 0) throw std::bad_alloc();
        return buf;
    }

    void deallocate(T* buf, std::size_t) { ::operator delete(buf); }
};

template <class T1, class T2>
bool operator==(const Mallocator<T1>& lhs, const Mallocator<T2>& rhs) {
    return true;
}

template <class T1, class T2>
bool operator!=(const Mallocator<T1>& lhs, const Mallocator<T2>& rhs) {
    return false;
}

#endif
