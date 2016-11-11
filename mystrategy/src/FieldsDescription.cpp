//
// Created by valdemar on 11.11.16.
//

#include "FieldsDescription.h"

namespace fields {

LinearRingField::LinearRingField(const geom::Point2D &center, double r1, double r2, double force)
: IVectorField(center), m_r1(r1), m_r2(r2), m_force(force) {

}

geom::Vec2D LinearRingField::apply_force(double x, double y) {
    geom::Vec2D v(m_center.x - x, m_center.y - y);
    double dist = v.len();
    if (dist >= m_r1 && dist <= m_r2) {
        return geom::normalize(v) * m_force;
    }
    return {0, 0};
}


ExpConfig ExpConfig::from_two_points(double value_on_zero, double force, double distance) {
    ExpConfig ret;
    ret.V = value_on_zero;
    ret.k = log(force / value_on_zero) / (-distance);
    return ret;
}

ExpRingField::ExpRingField(const geom::Point2D &center,
                           double r1,
                           double r2,
                           bool is_attractive,
                           const ExpConfig &conf)
: IVectorField(center), m_r1(r1), m_r2(r2), m_is_attractive(is_attractive), m_k(conf.k), m_V(conf.V) {
}


geom::Vec2D ExpRingField::apply_force(double x, double y) {
    geom::Vec2D v(m_center.x - x, m_center.y - y);
    double dist = v.len();
    if (dist >= m_r1 && dist <= m_r2) {
        double mod = exp(-x * m_k + log(m_V));
        if (!m_is_attractive) {
            mod *= -mod;
        }
        return geom::normalize(v) * mod;
    }
    return {0, 0};
}

}
