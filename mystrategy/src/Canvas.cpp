//
// Created by valdemar on 03.12.16.
//

#include "Canvas.h"

#include <memory.h>

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
