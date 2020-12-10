#pragma once

namespace util
{
    template<typename T>
    struct Movable
    {
        T& operator=(T&& oth)
        {
            static_cast<T*>(this)->~T();
            ::new (static_cast<void*>(this)) T(std::move(oth));
            return *static_cast<T*>(this);
        }
    };

    template<typename T>
    struct NullablePtr
    {
        T* ptr;

        NullablePtr() : ptr(nullptr)
        {
        }

        NullablePtr(const NullablePtr& oth) = default;

        NullablePtr(NullablePtr&& oth) : ptr(oth.ptr)
        {
            oth.ptr = nullptr;
        }

        NullablePtr(T* oth) : ptr(oth)
        {
        }

        operator T* () const
        {
            return ptr;
        }

        NullablePtr& operator=(NullablePtr&& oth)
        {
            ptr = oth.ptr;
            oth.ptr = nullptr;
            return *this;
        }

        NullablePtr& operator=(T* oth)
        {
            ptr = oth;
            return *this;
        }
    };
}