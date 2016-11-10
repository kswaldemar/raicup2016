//
// Created by valdemar on 09.11.16.
//

#pragma once

#include <cmath>

struct Point2D {

    constexpr Point2D(double x_, double y_)
        : x(static_cast<int>(x_)),
          y(static_cast<int>(y_)) {
    }

    constexpr Point2D(int x_, int y_)
        : x(x_),
          y(y_) {
    }

    Point2D operator+(const Point2D &other) const {
        return Point2D(x + other.x, y + other.y);
    }

    Point2D &operator+=(const Point2D &other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    Point2D &operator*=(const int scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    double get_distance_to(const Point2D &other) const {
        double xdiff = std::abs(other.x - x);
        double ydiff = std::abs(other.y - y);
        return  std::sqrt(xdiff * xdiff + ydiff * ydiff);
    }

    int x;
    int y;
};