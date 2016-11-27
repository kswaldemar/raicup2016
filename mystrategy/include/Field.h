//
// Created by valdemar on 11.11.16.
//

#pragma once

#include "Vec2D.h"


namespace fields {
/**
 * Абстрактный класс, для представления векторных полей.
 * В каждой точке определенная притягивает тело в центр с силой поля.
 * Если сила < 0 - поле отталкивающее
 */
class IField {
public:
    IField(const geom::Point2D &center)
        : m_center(center) {
    }

    /**
     * Применить силу поля к точке, находящейся в координатах (x; y)
     */
    virtual double get_value(double x, double y) const = 0;

protected:
    geom::Point2D m_center;
};

}
