//
// Created by valdemar on 09.11.16.
//

#pragma once

#include <cmath>

extern const double pi;

namespace geom {

inline constexpr double sqr(double x) {
    return x * x;
}

inline double rem(double x, double y) {
    return y * (x / y - floor(x / y));
}

inline double relAngle(double angle) {
    return rem(angle + pi, 2 * pi) - pi;
}


struct Vec2D {
    double x, y;

    Vec2D() {
    }

    constexpr Vec2D(const Vec2D &v)
        : x(v.x),
          y(v.y) {
    }

    constexpr Vec2D(double x_, double y_)
        : x(x_),
          y(y_) {
    }

    Vec2D &operator=(const Vec2D &v) {
        x = v.x;
        y = v.y;
        return *this;
    }

    constexpr Vec2D operator+(const Vec2D &v) const {
        return Vec2D(x + v.x, y + v.y);
    }

    Vec2D &operator+=(const Vec2D &v) {
        x += v.x;
        y += v.y;
        return *this;
    }

    constexpr Vec2D operator-(const Vec2D &v) const {
        return Vec2D(x - v.x, y - v.y);
    }

    Vec2D &operator-=(const Vec2D &v) {
        x -= v.x;
        y -= v.y;
        return *this;
    }

    constexpr Vec2D operator-() const {
        return Vec2D(-x, -y);
    }

    constexpr Vec2D operator*(double a) const {
        return Vec2D(a * x, a * y);
    }

    Vec2D &operator*=(double a) {
        x *= a;
        y *= a;
        return *this;
    }

    constexpr double operator*(const Vec2D &v) const {
        return x * v.x + y * v.y;
    }

    constexpr Vec2D operator/(double a) const {
        return (*this) * (1 / a);
    }

    Vec2D &operator/=(double a) {
        return (*this) *= (1 / a);
    }

    constexpr Vec2D operator~() const {
        return Vec2D(y, -x);
    }

    constexpr double operator%(const Vec2D &v) const {
        return *this * ~v;
    }

    constexpr double sqr() const {
        return x * x + y * y;
    }

    double len() const {
        return std::sqrt(x * x + y * y);
    }
};

inline constexpr Vec2D operator * (double a, const Vec2D &v)
{
    return v * a;
}

inline constexpr Vec2D normalize(const Vec2D &v)
{
    if (v.len() > 0) {
        return v / v.len();
    }
    return {0, 0};
}

inline Vec2D sincos(double angle)
{
    return Vec2D(cos(angle), sin(angle));
}

inline Vec2D sincos_fast(float angle)
{
    return Vec2D(cos(angle), sin(angle));
}

inline constexpr Vec2D rotate(const Vec2D &v1, const Vec2D &v2)
{
    return Vec2D(v1.x * v2.x - v1.y * v2.y, v1.x * v2.y + v1.y * v2.x);
}

inline constexpr Vec2D conj(const Vec2D &v)
{
    return Vec2D(v.x, -v.y);
}

//Same class, but only point in map, not vector by meaning
using Point2D = Vec2D;

}