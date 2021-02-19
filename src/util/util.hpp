#pragma once

#include <utility>
#include <cstdint>

namespace dry::util {

constexpr uint32_t popcount(uint32_t i) noexcept {
    // https://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
    // TODO : replace with intrinsics or fall back to this
    i = i - ((i >> 1) & 0x55555555);
    i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
    return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

// TODO : noexcepts probably don't work as intended fix
template<typename T>
struct movable {
    T& operator=(T&& oth) noexcept(std::is_nothrow_move_constructible_v<T>){
        static_cast<T*>(this)->~T();
        ::new (static_cast<void*>(this)) T(std::move(oth));
        return *static_cast<T*>(this);
    }
};

template<typename T>
struct assignable {
    T& operator=(T&& oth) noexcept(std::is_nothrow_move_constructible_v<T>) {
        static_cast<T*>(this)->~T();
        ::new (static_cast<void*>(this)) T(std::move(oth));
        return *static_cast<T*>(this);
    }
    T& operator=(const T& oth) noexcept(std::is_nothrow_copy_constructible_v<T>) {
        static_cast<T*>(this)->~T();
        ::new (static_cast<void*>(this)) T(oth);
        return *static_cast<T*>(this);
    }
};

template<typename T, T Null_Value>
struct nullable_base {
    T val;

    nullable_base() : val(Null_Value) {}
    nullable_base(const nullable_base&) = default;
    nullable_base(nullable_base&& oth) : val(oth.val) {
        oth.val = Null_Value;
    }
    explicit nullable_base(T oth) : val(oth) {}
    
    operator T const& () const {
        return val;
    }
    operator T& () {
        return val;
    }
    T const* operator&() const {
        return &val;
    }
    T* operator&() {
        return &val;
    }

    nullable_base& operator=(nullable_base&& oth) {
        val = oth.val;
        oth.val = Null_Value;
        return *this;
    }
    nullable_base& operator=(const T& oth) {
        val = oth;
        return *this;
    }
};

template<typename T>
struct nullable_ptr : public nullable_base<T*, nullptr> {
    using nullable_base<T*, nullptr>::nullable_base;
    using nullable_base<T*, nullptr>::operator=;

    T& operator*() const {
        return *(this->val);
    }
    T* operator->() const {
        return this->val;
    }
};

}

// TODO : don't like it in global, maybe can bring it to the scope of instantiation?
template<typename T>
consteval bool enable_flags(T) {
    return false;
};

template<typename T>
typename std::enable_if<enable_flags(T()), T>::type operator|(T l, T r) {
    using underlying = typename std::underlying_type<T>::type;
    return static_cast<T>(static_cast<underlying>(l) | static_cast<underlying>(r));
}

template<typename T>
typename std::enable_if<enable_flags(T()), T>::type operator&(T l, T r) {
    using underlying = typename std::underlying_type<T>::type;
    return static_cast<T>(static_cast<underlying>(l) & static_cast<underlying>(r));
}

template<typename T>
typename std::enable_if<enable_flags(T()), T>::type operator^(T l, T r) {
    using underlying = typename std::underlying_type<T>::type;
    return static_cast<T>(static_cast<underlying>(l) ^ static_cast<underlying>(r));
}

template<typename T>
typename std::enable_if<enable_flags(T()), T>::type operator~(T r) {
    using underlying = typename std::underlying_type<T>::type;
    return static_cast<T>(~static_cast<underlying>(r));
}

template<typename T>
typename std::enable_if<enable_flags(T()), T&>::type operator|=(T& l, T r) {
    using underlying = typename std::underlying_type<T>::type;
    return l = static_cast<T>(static_cast_<underlying>(l) | static_cast<underlying>(r));
}

template<typename T>
typename std::enable_if<enable_flags(T()), T&>::type& operator&=(T& l, T r) {
    using underlying = typename std::underlying_type<T>::type;
    return l = static_cast<T>(static_cast_<underlying>(l) & static_cast<underlying>(r));
}

template<typename T>
typename std::enable_if<enable_flags(T()), T&>::type& operator^=(T& l, T r) {
    using underlying = typename std::underlying_type<T>::type;
    return l = static_cast<T>(static_cast_<underlying>(l) ^ static_cast<underlying>(r));
}