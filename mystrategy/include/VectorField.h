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
class IVectorField {
public:
    IVectorField(const geom::Point2D &center)
        : m_center(center) {
    }

    /**
     * Применить силу векторного поля к точке, находящейся в координатах (x; y). Возвращает вектор поля.
     */
    virtual geom::Vec2D apply_force(double x, double y) = 0;

protected:
    geom::Point2D m_center;
};

}
