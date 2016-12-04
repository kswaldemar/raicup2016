//
// Created by valdemar on 11.11.16.
//

#pragma once

/**
 * Описание определенных полей, которые наследуются от IVectorField
 */
#include "PotentialField.h"

#include <memory>
#include <list>

namespace fields {

/**
 * Поле влияющее в диапазоне r1 <= x <= r2 с постоянной силой.
 */
class ConstField : public PotentialField {
public:
    ConstField(const geom::Point2D &center, double r1, double r2, double force);

    double calc_force(double sqr_x) const override;
protected:
    double m_force;
};


class LinearField : public PotentialField {
public:
    LinearField(const geom::Point2D &center, double r1, double r2, double force);

    double calc_force(double sqr_x) const override;
protected:
    double m_force;
};

/**
 * Конфигурация, для представления экспоненциальной силы
 * F = e ^ (-x * k + ln(V))
 */
struct ExpConfig {
    //Конфигурация по кривизне и опорной точке
    static ExpConfig from_two_points(double curvature, double y, double x);

    double k;
    double V;
};
/**
 * Поле влиющее в диапазоне r1 <= x <= r2 с силой, изменяющейся экспоненциально от расстояния
 */
class ExpRingField : public PotentialField {
public:
    ExpRingField(const geom::Point2D &center, double r1, double r2, bool is_attractive, const ExpConfig &conf);

    double calc_force(double sqr_x) const override;

protected:
    bool m_is_attractive;
    double m_k;
    double m_V;
};

}