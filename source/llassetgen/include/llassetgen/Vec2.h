#pragma once

#include <llassetgen/llassetgen_api.h>

namespace llassetgen {
    template <class T>
    struct Vec2 {
        T x = 0;
        T y = 0;

        Vec2() = default;
        Vec2(T _x, T _y) : x{_x}, y{_y} {}

        Vec2 operator+(const Vec2& other) const;
        Vec2& operator+=(const Vec2& other);
        Vec2 operator-(const Vec2& other) const;
        Vec2& operator-=(const Vec2& other);

        Vec2 operator-() const;

        bool operator==(const Vec2& other) const;
        bool operator!=(const Vec2& other) const;
    };

    template <class T>
    Vec2<T> Vec2<T>::operator+(const Vec2& other) const {
        return {x + other.x, y + other.y};
    }

    template <class T>
    Vec2<T>& Vec2<T>::operator+=(const Vec2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    template <class T>
    Vec2<T> Vec2<T>::operator-(const Vec2& other) const {
        return {x - other.x, y - other.y};
    }

    template <class T>
    Vec2<T>& Vec2<T>::operator-=(const Vec2& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    template <class T>
    Vec2<T> Vec2<T>::operator-() const {
        return {-x, -y};
    }

    template <class T>
    bool Vec2<T>::operator==(const Vec2& other) const {
        return x == other.x && y == other.y;
    }

    template <class T>
    bool Vec2<T>::operator!=(const Vec2& other) const {
        return !(*this == other);
    }
}
