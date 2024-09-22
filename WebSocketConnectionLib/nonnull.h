#pragma once

#include <cstddef>
#include <type_traits>

template <typename T, typename U = std::enable_if_t<std::is_pointer_v<T>, T>>
class NonNull {
public:
    NonNull(std::nullptr_t) = delete;

    NonNull(T object) : obj(object) {};

    T operator=(std::nullptr_t) = delete;
    T operator=(T object) {
        obj = object;
    }

    operator T() const { return obj; }
    T operator->() const { return obj; }
    T operator*() const { return obj; }

private:
    T obj = nullptr;
};
