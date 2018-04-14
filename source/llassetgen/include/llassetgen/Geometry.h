#pragma once

#include <algorithm>
#include <ostream>

template <typename T>
T square(T value) {
    return value * value;
}

template <typename T>
T clamp(T val, T min, T max) {
    return std::max(min, std::min(max, val));
}

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
        Vec2 operator/(const T dividend) const;
        Vec2& operator/=(const T dividend);

        Vec2 operator-() const;

        bool operator==(const Vec2& other) const;
        bool operator!=(const Vec2& other) const;
    };

    template <class T>
    struct Rect {
        Vec2<T> position{0, 0};
        Vec2<T> size{0, 0};

        Rect() = default;
        Rect(const Vec2<T>& _position, const Vec2<T>& _size);

        bool contains(const Rect& other) const;
        bool overlaps(const Rect& other) const;

        bool operator==(const Rect& other) const;
        bool operator!=(const Rect& other) const;
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

    template<class T>
    Vec2<T> Vec2<T>::operator/(const T dividend) const {
        return {x / dividend, y / dividend};
    }

    template<class T>
    Vec2<T>& Vec2<T>::operator/=(const T dividend) {
        x /= dividend;
        y /= dividend;
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

    template <class T>
    Rect<T>::Rect(const Vec2<T>& _position, const Vec2<T>& _size) : position{_position}, size{_size} {}

    template <class T>
    bool Rect<T>::operator==(const Rect& other) const {
        return position == other.position && size == other.size;
    }

    template <class T>
    bool Rect<T>::operator!=(const Rect& other) const {
        return !(*this == other);
    }

    template <class T>
    bool Rect<T>::contains(const Rect<T>& other) const {
        auto thisMaxPoint = position + size;
        auto otherMaxPoint = other.position + other.size;
        return position.x <= other.position.x && position.y <= other.position.y && thisMaxPoint.x >= otherMaxPoint.x &&
               thisMaxPoint.y >= otherMaxPoint.y;
    }

    template <class T>
    bool Rect<T>::overlaps(const Rect<T>& other) const {
        return position.x < other.position.x + other.size.x && position.x + size.x > other.position.x &&
               position.y < other.position.y + other.size.y && position.y + size.y > other.position.y;
    }

    template <class T>
    std::ostream& operator<<(std::ostream& out, const Vec2<T>& vec) {
        out << '(' << vec.x << ',' << vec.y << ')';
        return out;
    }

    template <class T>
    std::ostream& operator<<(std::ostream& out, const Rect<T>& rect) {
        out << "Rect(pos: " << rect.position << ", size: " << rect.size << ')';
        return out;
    }
}
