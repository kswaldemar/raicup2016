//
// Created by valdemar on 04.12.16.
//

#pragma once

#include "Vec2D.h"

class MyBonus {
public:

    MyBonus(double x, double y, int last_update);

    int getRemainingTime() const;

    void update(int tick);

    void setTaken();

    double getX() const;

    double getY() const;

    geom::Point2D getPoint() const;

private:
    double m_x;
    double m_y;
    int m_last_update_tick;
    int m_next_time;
};


