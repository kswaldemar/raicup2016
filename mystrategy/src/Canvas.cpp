//
// Created by valdemar on 03.12.16.
//

#include "Canvas.h"

#include <memory.h>
#include <algorithm>

Canvas::Canvas() {
    clear();
}

void Canvas::clear() {
    memset(m_canvas, 0, CELL_COUNT * CELL_COUNT);
}

geom::CellCoord Canvas::to_internal(double x, double y) const {
    return geom::CellCoord(static_cast<int>(round(x / GRID_SIZE)), static_cast<int>(round(y / GRID_SIZE)));
}

int Canvas::to_internal(double len) const {
    return static_cast<int>(ceil(len / GRID_SIZE));
}

void Canvas::draw_circle(const geom::CellCoord &pt, int radius) {
    int x = radius;
    int y = 0;
    int decision = 1 - x;

    while (x >= y) {

        put_pixel(pt.x + x, pt.y + y);
        put_pixel(pt.x + y, pt.y + x);
        put_pixel(pt.x - y, pt.y + x);
        put_pixel(pt.x - x, pt.y + y);
        put_pixel(pt.x - x, pt.y - y);
        put_pixel(pt.x - y, pt.y - x);
        put_pixel(pt.x + y, pt.y - x);
        put_pixel(pt.x + x, pt.y - y);

        ++y;
        if (decision <= 0) {
            decision += 2 * y + 1;
        } else {
            --x;
            decision += 2 * (y - x) + 1;
        }
    }
}

void Canvas::put_pixel(const geom::CellCoord &pt) {
    put_pixel(pt.x, pt.y);
}

void Canvas::put_pixel(int x, int y) {
    if (is_correct_point(x, y)) {
        m_canvas[x][y] = true;
    }
}

bool Canvas::get_pixel(const geom::CellCoord &pt) const {
    return get_pixel(pt.x, pt.y);
}

bool Canvas::get_pixel(int x, int y) const {
    if (is_correct_point(x, y)) {
        return m_canvas[x][y];
    }
    return true;
}

bool Canvas::is_correct_point(const geom::CellCoord &pt) const {
    return is_correct_point(pt.x, pt.y);
}

bool Canvas::is_correct_point(int x, int y) const {
    return x >= 0 && x < CELL_COUNT && y >= 0 && y < CELL_COUNT;
}

bool Canvas::check_circle_intersection(int x0, int y0, int radius) const {
    int x = radius;
    int y = 0;
    int decision = 1 - x;
    bool intersected = false;
    while (x >= y && !intersected) {

        intersected = intersected || get_pixel(x0 + x, y0 + y);
        intersected = intersected || get_pixel(x0 + y, y0 + x);
        intersected = intersected || get_pixel(x0 - y, y0 + x);
        intersected = intersected || get_pixel(x0 - x, y0 + y);
        intersected = intersected || get_pixel(x0 - x, y0 - y);
        intersected = intersected || get_pixel(x0 - y, y0 - x);
        intersected = intersected || get_pixel(x0 + y, y0 - x);
        intersected = intersected || get_pixel(x0 + x, y0 - y);

        ++y;
        if (decision <= 0) {
            decision += 2 * y + 1;
        } else {
            --x;
            decision += 2 * (y - x) + 1;
        }
    }
    return intersected;
}

bool Canvas::check_line_intersection(int x1, int y1, int x2, int y2) const {
    //Bresenham's line algorithm
    int w = x2 - x1;
    int h = y2 - y1;
    int dx1 = 0;
    int dy1 = 0;
    int dx2 = 0;
    int dy2 = 0;
    if (w != 0) {
        dx1 = w < 0 ? -1 : 1;
        dx2 = w < 0 ? -1 : 1;
    }
    if (h != 0) {
        dy1 = h < 0 ? -1 : 1;
    }

    int longest = std::abs(w);
    int shortest = std::abs(h);
    if (longest <= shortest) {
        std::swap(longest, shortest);
        if (h < 0) {
            dy2 = -1;
        } else if (h > 0) {
            dy2 = 1;
        }
        dx2 = 0;
    }
    int numerator = longest >> 1;
    int x = x1;
    int y = y1;
    bool has_pixel = false;
    for (int i = 0; i <= longest && !has_pixel; i++) {

        has_pixel = get_pixel(x, y);

        numerator += shortest;
        if (numerator >= longest) {
            numerator -= longest;
            x += dx1;
            //Bold line check
            has_pixel = has_pixel || get_pixel(x, y);
            y += dy1;
        } else {
            x += dx2;
            has_pixel = has_pixel || get_pixel(x, y);
            y += dy2;
        }
    }
    return !has_pixel;
}
