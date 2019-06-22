#ifndef NAIVEALLOCATOR_HPP
#define NAIVEALLOCATOR_HPP

#include <climits>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <new>

template <class T>
class Nallocator {
   public:
    typedef T value_type;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;

    Nallocator() = default;

    template <class U>
    Nallocator(const Nallocator<U>& other) noexcept;

    T* allocate(std::size_t n) {
        auto buf = (T*)(::operator new((std::size_t)(n * sizeof(T))));
        if (buf == 0) throw std::bad_alloc();
        return buf;
    }

    void deallocate(T* buf, std::size_t) { ::operator delete(buf); }
};

template <class T1, class T2>
bool operator==(const Nallocator<T1>& lhs, const Nallocator<T2>& rhs) {
    return true;
}

template <class T1, class T2>
bool operator!=(const Nallocator<T1>& lhs, const Nallocator<T2>& rhs) {
    return false;
}

#endif
