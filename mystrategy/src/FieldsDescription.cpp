//
// Created by valdemar on 11.11.16.
//

#include <cassert>
#include "FieldsDescription.h"

namespace fields {

ConstRingField::ConstRingField(const geom::Point2D &center, double r1, double r2, double force)
: IVectorField(center), m_r1(r1), m_r2(r2), m_force(force) {
    assert(r1 >= 0);
    assert(r2 >= 0);
}

geom::Vec2D ConstRingField::apply_force(double x, double y) const {
    geom::Vec2D v(m_center.x - x, m_center.y - y);
    double dist = v.len();
    if (dist >= m_r1 && dist <= m_r2) {
        return geom::normalize(v) * m_force;
    }
    return {0, 0};
}


ExpConfig ExpConfig::from_two_points(double curvature, double y, double x) {
    ExpConfig ret;
    ret.k = curvature;
    ret.V = y * exp(x * curvature);
    return ret;
}

ExpRingField::ExpRingField(const geom::Point2D &center,
                           double r1,
                           double r2,
                           bool is_attractive,
                           const ExpConfig &conf)
: IVectorField(center), m_r1(r1), m_r2(r2), m_is_attractive(is_attractive), m_k(conf.k), m_V(conf.V) {
    assert(r1 >= 0);
    assert(r2 >= 0);
}


geom::Vec2D ExpRingField::apply_force(double x, double y) const {
    geom::Vec2D v(m_center.x - x, m_center.y - y);
    double dist = v.len();
    if (dist >= m_r1 && dist <= m_r2) {
        double mod = m_V * exp(-dist * m_k);
        if (!m_is_attractive) {
            mod = -mod;
        }
        return geom::normalize(v) * mod;
    }
    return {0, 0};
}

}
