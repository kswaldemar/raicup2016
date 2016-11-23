//
// Created by valdemar on 11.11.16.
//

#pragma once

/**
 * Описание определенных полей, которые наследуются от IVectorField
 */
#include "VectorField.h"

#include <memory>
#include <list>

namespace fields {

/**
 * Поле влияющее в диапазоне r1 <= x <= r2 с постоянной силой.
 */
class ConstRingField : public IField {
public:
    ConstRingField(const geom::Point2D &center, double r1, double r2, double force);

    double apply_force(double x, double y) const override;
protected:
    double m_r1;
    double m_r2;
    double m_force;
};


class LinearField : public IField {
public:
    LinearField(const geom::Point2D &center, double r1, double r2, double force);

    double apply_force(double x, double y) const override;
protected:
    double m_r1;
    double m_r2;
    double m_force;
};

/**
 * Поле влияющие в диапазоне r1 <= x <= r2 с силой, изменяющейся линейно от расстояния
 */
//TODO: Implement
//class LinearRingField : public IVectorField {
//
//};

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
class ExpRingField : public IField {
public:
    ExpRingField(const geom::Point2D &center, double r1, double r2, bool is_attractive, const ExpConfig &conf);

    double apply_force(double x, double y) const override;

protected:
    double m_r1;
    double m_r2;
    bool m_is_attractive;
    double m_k;
    double m_V;
};

}