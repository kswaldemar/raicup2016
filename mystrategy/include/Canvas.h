//
// Created by valdemar on 03.12.16.
//

#pragma once

#include "Vec2D.h"

class Canvas {
public:
    static constexpr int MAP_SIZE = 1200;
    static constexpr int GRID_SIZE = 5;
    static constexpr int CELL_COUNT = (MAP_SIZE + GRID_SIZE - 1) / GRID_SIZE;

    Canvas();

    void clear();

    geom::CellCoord to_internal(double x, double y) const;

    int to_internal(double len) const;

    void draw_circle(const geom::CellCoord &pt, int radius);

    void put_pixel(const geom::CellCoord &pt);

    void put_pixel(int x, int y);

    bool get_pixel(const geom::CellCoord &pt) const;

    bool get_pixel(int x, int y) const;

    bool is_correct_point(const geom::CellCoord &pt) const;

private:

    bool is_correct_point(int x, int y) const;

    bool m_canvas[CELL_COUNT][CELL_COUNT];
};