//
// Created by valdemar on 09.11.16.
//

#pragma once

#include <cmath>

constexpr double pi = 3.14159265358979323846;

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

template<typename T>
struct CustomVec2D {
    T x, y;

    CustomVec2D() {
    }

    constexpr CustomVec2D(const CustomVec2D &v)
        : x(v.x),
          y(v.y) {
    }

    constexpr CustomVec2D(double x_, double y_)
        : x(x_),
          y(y_) {
    }

    CustomVec2D &operator=(const CustomVec2D &v) {
        x = v.x;
        y = v.y;
        return *this;
    }

    constexpr CustomVec2D operator+(const CustomVec2D &v) const {
        return CustomVec2D(x + v.x, y + v.y);
    }

    CustomVec2D &operator+=(const CustomVec2D &v) {
        x += v.x;
        y += v.y;
        return *this;
    }

    constexpr CustomVec2D operator-(const CustomVec2D &v) const {
        return CustomVec2D(x - v.x, y - v.y);
    }

    CustomVec2D &operator-=(const CustomVec2D &v) {
        x -= v.x;
        y -= v.y;
        return *this;
    }

    constexpr CustomVec2D operator-() const {
        return CustomVec2D(-x, -y);
    }

    constexpr CustomVec2D operator*(double a) const {
        return CustomVec2D(a * x, a * y);
    }

    CustomVec2D &operator*=(double a) {
        x *= a;
        y *= a;
        return *this;
    }

    constexpr double operator*(const CustomVec2D &v) const {
        return x * v.x + y * v.y;
    }

    constexpr CustomVec2D operator/(double a) const {
        return (*this) * (1 / a);
    }

    CustomVec2D &operator/=(double a) {
        return (*this) *= (1 / a);
    }

    constexpr CustomVec2D operator~() const {
        return CustomVec2D(y, -x);
    }

    constexpr double operator%(const CustomVec2D &v) const {
        return *this * ~v;
    }

    constexpr double sqr() const {
        return x * x + y * y;
    }

    double len() const {
        return std::sqrt(x * x + y * y);
    }
};

using Vec2D = CustomVec2D<double>;
//Same class, but only point in map, not vector by meaning
using Point2D = Vec2D;
using CellCoord = CustomVec2D<int>;


inline constexpr Vec2D operator * (double a, const Vec2D &v)
{
    return v * a;
}

inline const Vec2D normalize(const Vec2D &v)
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

inline constexpr double rad_to_deg(const double rad_angle) {
    return rad_angle * 180.0 / pi;
}

inline constexpr double deg_to_rad(const double deg_angle) {
    return deg_angle * pi / 180.0;
}


}