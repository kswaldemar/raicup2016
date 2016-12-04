//
// Created by valdemar on 11.11.16.
//

#pragma once

#include "Vec2D.h"


namespace fields {

/**
 * Potential field with influence from r1 to r2 distance to center
 */
class PotentialField {
public:

    PotentialField(const geom::Point2D &center, double r1, double r2);

    double get_value(const geom::Point2D &pt) const;

    double get_value(double x, double y) const;

    virtual double calc_force(double sqr_x) const = 0;

protected:
    geom::Point2D m_center;
    double m_r1;
    double m_r2;
};

}
